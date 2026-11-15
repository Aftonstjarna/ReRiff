"""ReRiff balance policy. Native reward/unlock decisions remain the input.

This module never accepts an arbitrary client balance or achievement count as
proof of progress. The initial endpoint publishes policy only; earning/reset
endpoints must wait for verified game integration and authenticated sessions.
"""
import copy
from decimal import Decimal, InvalidOperation, ROUND_FLOOR
import hashlib
import json
import os
from pathlib import Path
import re

DEFAULT = Path(__file__).with_name('balance.json')
MAX_COUNTER = 2147483647
ENV_PREFIX = 'RERIFF_BALANCE_'

# Names mirror [balance] in singleplayer.ini. Server-only policy fields use
# the same prefix; the server never reads a player's local INI or save.
ENV_FIELDS = {
    'SEASON': ('season',),
    'ANTI_CHEAT': ('anti_cheat',),
    'MAIN_XP_RATE': ('xp', 'main_rate'),
    'PRESTIGE_XP_RATE': ('xp', 'prestige_rate'),
    'COIN_REWARD_RATE': ('coins', 'base_rate'),
    'CREATOR_REWARD_RATE': ('coins', 'creator_rate'),
    'PRESTIGE_MULTIPLIER_GROWTH': ('coins', 'prestige_growth'),
    'PRESTIGE_MULTIPLIER_CAP': ('coins', 'multiplier_cap'),
    'PAINT_PRICE_SCALE': ('unlocks', 'paint_price_scale'),
    'DISCOUNT_MODE': ('discounts', 'mode'),
    'DISCOUNT_START_LEVEL': ('discounts', 'start_level'),
    'DISCOUNT_START': ('discounts', 'start'),
    'DISCOUNT_INCREASE': ('discounts', 'increase'),
    'DISCOUNT_INTERVAL': ('discounts', 'diminishing_interval'),
    'DISCOUNT_MAXIMUM': ('discounts', 'maximum'),
    'DISCOUNT_LEVEL_10': ('discounts', 'milestones', 0, 'fraction'),
    'DISCOUNT_LEVEL_20': ('discounts', 'milestones', 1, 'fraction'),
    'DISCOUNT_LEVEL_30': ('discounts', 'milestones', 2, 'fraction'),
    'DISCOUNT_REMAINING_GAP_FACTOR': ('discounts', 'remaining_gap_factor'),
    'MAIN_LEVEL_CAP': ('main_levels', 'cap'),
    'MAIN_LEVEL_XP_STEP': ('main_levels', 'xp_step'),
    'MAIN_LEVEL_XP_GROWTH': ('main_levels', 'xp_growth'),
    'CAR_LEVELS': ('unlocks', 'car_levels'),
    'REQUIRED_ACHIEVEMENTS': ('prestige', 'required_achievements'),
    'MAXIMUM_PRESTIGE_COUNT': ('prestige', 'maximum_count'),
}


class PolicyError(ValueError):
    pass


def integer(value, label, minimum=0, maximum=MAX_COUNTER):
    if type(value) is not int or not minimum <= value <= maximum:
        raise PolicyError(label+' must be an integer in range')
    return value


def fraction(value, label, minimum=Decimal(0), maximum=Decimal(1000000)):
    if isinstance(value, bool) or not isinstance(value, (str, int, float, Decimal)):
        raise PolicyError(label+' must be a finite decimal')
    try:
        number = Decimal(str(value))
    except InvalidOperation:
        raise PolicyError(label+' must be a finite decimal') from None
    if not number.is_finite() or not minimum <= number <= maximum or number.as_tuple().exponent < -6:
        raise PolicyError(label+' is out of range or has more than six decimal places')
    return number


def keys(value, expected, label):
    if not isinstance(value, dict) or set(value) != set(expected):
        raise PolicyError(label+' has missing or unknown settings')


def decimal_text(value):
    return format(value.normalize(), 'f')


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise PolicyError('Duplicate balance setting')
        result[key] = value
    return result


class Balance:
    def __init__(self, config):
        config = copy.deepcopy(config)
        # Keep explicit loads of the earlier preview preset readable.
        if isinstance(config, dict):
            config.setdefault('main_levels', {'cap': 0, 'xp_step': 0, 'xp_growth': '1'})
            if isinstance(config.get('coins'), dict):
                config['coins'].setdefault('creator_rate', config['coins'].get('base_rate', '1'))
            if isinstance(config.get('discounts'), dict):
                for key, value in {'mode': 'legacy', 'start_level': 10, 'start': '0.10', 'increase': '0.05'}.items():
                    config['discounts'].setdefault(key, value)
        keys(config, ['schema', 'season', 'anti_cheat', 'xp', 'coins', 'discounts', 'unlocks', 'prestige', 'main_levels'], 'balance')
        if type(config['schema']) is not int or config['schema'] != 1:
            raise PolicyError('Unsupported balance schema')
        if not isinstance(config['season'], str) or not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', config['season']):
            raise PolicyError('Invalid season identifier')
        if config['anti_cheat'] not in ('disabled', 'required'):
            raise PolicyError('anti_cheat must be disabled or required')
        keys(config['xp'], ['main_rate', 'prestige_rate'], 'xp')
        keys(config['coins'], ['base_rate', 'creator_rate', 'prestige_growth', 'multiplier_cap'], 'coins')
        keys(config['discounts'], ['mode', 'start_level', 'start', 'increase', 'milestones', 'maximum', 'diminishing_interval', 'remaining_gap_factor'], 'discounts')
        keys(config['main_levels'], ['cap', 'xp_step', 'xp_growth'], 'main_levels')
        keys(config['unlocks'], ['car_levels', 'paint_price_scale'], 'unlocks')
        keys(config['prestige'], ['required_achievements', 'maximum_count'], 'prestige')
        for group, names in [('xp', ['main_rate', 'prestige_rate']), ('coins', ['base_rate', 'creator_rate', 'prestige_growth']), ('unlocks', ['paint_price_scale'])]:
            for name in names:
                config[group][name] = decimal_text(fraction(config[group][name], group+'.'+name))
        cap = config['coins']['multiplier_cap']
        if cap is not None:
            config['coins']['multiplier_cap'] = decimal_text(fraction(cap, 'coins.multiplier_cap', Decimal(1)))
        discounts = config['discounts']
        maximum = fraction(discounts['maximum'], 'discounts.maximum', maximum=Decimal('0.99'))
        discounts['maximum'] = decimal_text(maximum)
        if discounts['mode'] not in ('legacy', 'interval'):
            raise PolicyError('discounts.mode must be legacy or interval')
        integer(discounts['start_level'], 'discounts.start_level')
        discounts['start'] = decimal_text(fraction(discounts['start'], 'discounts.start', maximum=maximum))
        discounts['increase'] = decimal_text(fraction(discounts['increase'], 'discounts.increase', maximum=Decimal('.99')))
        factor = fraction(discounts['remaining_gap_factor'], 'discounts.remaining_gap_factor', maximum=Decimal(1))
        discounts['remaining_gap_factor'] = decimal_text(factor)
        integer(discounts['diminishing_interval'], 'discounts.diminishing_interval', 1, 1000000)
        milestones = discounts['milestones']
        if not isinstance(milestones, list) or not 1 <= len(milestones) <= 128:
            raise PolicyError('Provide 1–128 discount milestones')
        previous_level, previous_discount = -1, Decimal(0)
        for milestone in milestones:
            keys(milestone, ['level', 'fraction'], 'milestone')
            level = integer(milestone['level'], 'milestone.level', 0, 1000000)
            discount = fraction(milestone['fraction'], 'milestone.fraction',
                                maximum=maximum if discounts['mode'] == 'legacy' else Decimal('.99'))
            if level <= previous_level or discount < previous_discount:
                raise PolicyError('Discount milestones must increase in level and not decrease in value')
            milestone['fraction'] = decimal_text(discount)
            previous_level, previous_discount = level, discount
        levels = config['main_levels']
        cap = integer(levels['cap'], 'main_levels.cap')
        if 0 < cap < 40:
            raise PolicyError('Main level cap must be 0 or at least 40')
        integer(levels['xp_step'], 'main_levels.xp_step', maximum=2**64-1)
        levels['xp_growth'] = decimal_text(fraction(levels['xp_growth'], 'main_levels.xp_growth', Decimal(1)))
        cars = config['unlocks']['car_levels']
        if not isinstance(cars, dict):
            raise PolicyError('unlocks.car_levels must be a mapping')
        for car, level in cars.items():
            if not isinstance(car, str) or not re.fullmatch(r'(?:[0-9]|1[0-9]|2[01])', car):
                raise PolicyError('Car slot must be 0–21')
            integer(level, 'car level', 0, 1000000)
        required = config['prestige']['required_achievements']
        if required != 'native_all':
            if not isinstance(required, list) or not required:
                raise PolicyError('Prestige requires native_all or distinct native achievement IDs')
            for item in required:
                integer(item, 'achievement ID', 0, 38)
            if len(set(required)) != len(required):
                raise PolicyError('Prestige achievement IDs must be distinct')
            config['prestige']['required_achievements'] = sorted(required)
        count = config['prestige']['maximum_count']
        if count is not None:
            integer(count, 'prestige.maximum_count')
        self._config = config
        self.revision = hashlib.sha256(json.dumps(config, sort_keys=True, separators=(',', ':'), ensure_ascii=True).encode()).hexdigest()

    @classmethod
    def load(cls, path=None):
        path = Path(path) if path is not None else DEFAULT
        try:
            if path.stat().st_size > 65536:
                raise PolicyError('Balance policy exceeds 64 KiB')
            return cls(json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=unique_object))
        except (OSError, json.JSONDecodeError) as error:
            raise PolicyError('Could not read balance policy: '+type(error).__name__) from error

    @classmethod
    def from_env(cls, environ=None):
        """Packaged defaults plus explicit environment overrides; no data-file overrides."""
        environ = os.environ if environ is None else environ
        config = cls.load().config
        for name, raw in environ.items():
            if not name.startswith(ENV_PREFIX):
                continue
            key = name[len(ENV_PREFIX):]
            if key not in ENV_FIELDS:
                raise PolicyError('Unknown balance environment setting: '+name)
            if not isinstance(raw, str) or not raw.strip() or len(raw) > 65536:
                raise PolicyError('Empty or oversized balance environment setting: '+name)
            value = raw.strip()
            if key in ('DISCOUNT_START_LEVEL', 'DISCOUNT_INTERVAL', 'MAIN_LEVEL_CAP', 'MAIN_LEVEL_XP_STEP', 'MAXIMUM_PRESTIGE_COUNT'):
                if key == 'MAXIMUM_PRESTIGE_COUNT' and value == 'null':
                    value = None
                elif re.fullmatch(r'[0-9]{1,20}', value):
                    value = int(value)
                else:
                    raise PolicyError('Expected a whole number for '+name)
            elif key == 'PRESTIGE_MULTIPLIER_CAP':
                value = fraction(value, name)
                value = None if value == 0 else value
            elif key == 'CAR_LEVELS' or (key == 'REQUIRED_ACHIEVEMENTS' and value != 'native_all'):
                try:
                    value = json.loads(value, object_pairs_hook=unique_object)
                except (ValueError, RecursionError) as error:
                    raise PolicyError('Invalid JSON for '+name) from error
            path = ENV_FIELDS[key]
            target = config
            for part in path[:-1]:
                target = target[part]
            target[path[-1]] = value
        return cls(config)

    @property
    def config(self):
        return copy.deepcopy(self._config)

    def coin_multiplier(self, prestige_count):
        integer(prestige_count, 'prestige count')
        multiplier = Decimal(1)+Decimal(self._config['coins']['prestige_growth'])*prestige_count
        cap = self._config['coins']['multiplier_cap']
        return min(multiplier, Decimal(cap)) if cap is not None else multiplier

    @staticmethod
    def _reward(base, rate):
        integer(base, 'native reward')
        return min(MAX_COUNTER, int((Decimal(base)*rate).to_integral_value(rounding=ROUND_FLOOR)))

    def coin_reward(self, native_reward, prestige_count, *, creator=False):
        rate = self._config['coins']['creator_rate' if creator else 'base_rate']
        return self._reward(native_reward, Decimal(rate)*self.coin_multiplier(prestige_count))

    def xp_reward(self, native_reward, *, prestige=False):
        return self._reward(native_reward, Decimal(self._config['xp']['prestige_rate' if prestige else 'main_rate']))

    def discount(self, main_level):
        integer(main_level, 'main level')
        settings = self._config['discounts']
        if settings['mode'] == 'interval':
            if main_level < settings['start_level']:
                return Decimal(0)
            steps = (main_level-settings['start_level'])//settings['diminishing_interval']
            return min(Decimal(settings['maximum']), Decimal(settings['start'])+steps*Decimal(settings['increase']))
        result = Decimal(0)
        for milestone in settings['milestones']:
            if main_level < milestone['level']:
                break
            result = Decimal(milestone['fraction'])
        last = settings['milestones'][-1]
        if main_level >= last['level']:
            steps = (main_level-last['level'])//settings['diminishing_interval']
            maximum = Decimal(settings['maximum'])
            result = maximum-(maximum-result)*Decimal(settings['remaining_gap_factor'])**steps
        return result

    def paint_price(self, native_price, main_level):
        integer(native_price, 'native price')
        scaled = Decimal(native_price)*Decimal(self._config['unlocks']['paint_price_scale'])*(1-self.discount(main_level))
        return min(MAX_COUNTER, max(0 if native_price == 0 else 1, int(scaled.to_integral_value(rounding=ROUND_FLOOR))))

    def car_requirement(self, car, native_level):
        integer(car, 'car slot', 0, 21)
        integer(native_level, 'native car level')
        return self._config['unlocks']['car_levels'].get(str(car), native_level)

    def prestige_eligible(self, achieved, count):
        integer(count, 'prestige count')
        if not isinstance(achieved, (set, list, tuple)):
            raise PolicyError('Native achievement IDs are required')
        for item in achieved:
            integer(item, 'achievement ID', 0, 38)
        maximum = self._config['prestige']['maximum_count']
        required = self._config['prestige']['required_achievements']
        required = (set(range(39)) - {18}) if required == 'native_all' else set(required)
        return (maximum is None or count < maximum) and required.issubset(set(achieved))


def register_endpoint(app, data_dir, server_id):
    """Read-only beta capability. No profile writes, resets or client rewards."""
    from flask import jsonify
    # Capture the process environment at application creation. Recreate a
    # container after editing its env file; /data/balance.json is not consulted.
    environ = {name: value for name, value in os.environ.items() if name.startswith(ENV_PREFIX)}

    @app.get('/reriff/balance')
    def reriff_balance():
        try:
            policy = Balance.from_env(environ)
            response = jsonify({'schema': 1, 'serverId': server_id, 'balanceRevision': policy.revision,
                'balance': policy.config, 'capabilities': {'policyPreview': True, 'progressionEnforced': False,
                'prestigeReset': False, 'antiCheatVerified': False}})
            response.headers['Cache-Control'] = 'no-store'
            return response
        except PolicyError:
            # Do not silently substitute easier rules or leak local paths.
            return jsonify({'error': 'Invalid server balance policy'}), 503
