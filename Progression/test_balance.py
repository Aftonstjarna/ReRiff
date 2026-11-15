from decimal import Decimal
import configparser
import json
import os
from pathlib import Path
import tempfile
import unittest
import uuid
from unittest.mock import patch
from flask import Flask

from balance import Balance, PolicyError, register_endpoint


class BalanceTests(unittest.TestCase):
    def setUp(self):
        environment = patch.dict(os.environ, {key: value for key, value in os.environ.items()
                                             if not key.startswith('RERIFF_BALANCE_')}, clear=True)
        environment.start()
        self.addCleanup(environment.stop)
        self.balance = Balance.load()

    def test_default_coin_multiplier_and_xp_rates(self):
        for count in range(4):
            self.assertEqual(self.balance.coin_reward(100, count), 100*(count+1))
        self.assertEqual(self.balance.xp_reward(150), 150)
        self.assertEqual(self.balance.xp_reward(150, prestige=True), 150)

    def test_permanent_level_discounts_and_native_price(self):
        for level, expected in [(0, '0'), (9, '0'), (10, '.2'), (14, '.2'), (15, '.25'),
                                (20, '.3'), (25, '.35'), (30, '.4'), (35, '.45'), (40, '.5'), (50, '.5')]:
            self.assertEqual(self.balance.discount(level), Decimal(expected))
        self.assertEqual(self.balance.paint_price(10000, 30), 6000)
        self.assertEqual(self.balance.paint_price(0, 30), 0)
        self.assertEqual(self.balance.paint_price(1, 100), 1)
        self.assertEqual(self.balance.discount(2147483647), Decimal('.50'))

    def test_legacy_discount_curve_remains_selectable(self):
        balance = Balance.from_env({'RERIFF_BALANCE_DISCOUNT_MODE': 'legacy',
                                    'RERIFF_BALANCE_DISCOUNT_MAXIMUM': '0.30',
                                    'RERIFF_BALANCE_DISCOUNT_INTERVAL': '10'})
        for level, expected in [(9, '0'), (10, '.1'), (20, '.2'), (30, '.25'), (40, '.275'), (50, '.2875')]:
            self.assertEqual(balance.discount(level), Decimal(expected))

    def test_custom_growth_cap_rates_requirements_and_prices(self):
        config = self.balance.config
        config['coins'].update(base_rate='1.5', prestige_growth='.25', multiplier_cap='2')
        config['xp'].update(main_rate='2', prestige_rate='.5')
        config['unlocks'].update(car_levels={'5': 15}, paint_price_scale='2')
        balance = Balance(config)
        self.assertEqual(balance.coin_reward(100, 100), 300)
        self.assertEqual(balance.xp_reward(100), 200)
        self.assertEqual(balance.xp_reward(100, prestige=True), 50)
        self.assertEqual(balance.car_requirement(5, 20), 15)
        self.assertEqual(balance.car_requirement(6, 20), 20)
        self.assertEqual(balance.paint_price(100, 30), 120)

    def test_environment_overrides_rewards_prices_levels_and_policy(self):
        balance = Balance.from_env({
            'RERIFF_BALANCE_COIN_REWARD_RATE': '1.5',
            'RERIFF_BALANCE_CREATOR_REWARD_RATE': '2',
            'RERIFF_BALANCE_PRESTIGE_MULTIPLIER_GROWTH': '.25',
            'RERIFF_BALANCE_PRESTIGE_MULTIPLIER_CAP': '2',
            'RERIFF_BALANCE_MAIN_XP_RATE': '2',
            'RERIFF_BALANCE_PRESTIGE_XP_RATE': '.5',
            'RERIFF_BALANCE_PAINT_PRICE_SCALE': '.5',
            'RERIFF_BALANCE_MAIN_LEVEL_CAP': '100',
            'RERIFF_BALANCE_MAIN_LEVEL_XP_STEP': '18446744073709551615',
            'RERIFF_BALANCE_MAIN_LEVEL_XP_GROWTH': '1.1',
            'RERIFF_BALANCE_CAR_LEVELS': '{"5":15}',
            'RERIFF_BALANCE_REQUIRED_ACHIEVEMENTS': '[1,0]',
            'RERIFF_BALANCE_MAXIMUM_PRESTIGE_COUNT': '2',
            'RERIFF_BALANCE_SEASON': 'test-season',
            'RERIFF_BALANCE_ANTI_CHEAT': 'required',
        })
        self.assertEqual(balance.coin_reward(100, 100), 300)
        self.assertEqual(balance.coin_reward(100, 100, creator=True), 400)
        self.assertEqual(balance.xp_reward(100), 200)
        self.assertEqual(balance.xp_reward(100, prestige=True), 50)
        self.assertEqual(balance.paint_price(1000, 10), 400)
        self.assertEqual(balance.car_requirement(5, 20), 15)
        self.assertEqual(balance.config['main_levels'], {'cap': 100, 'xp_step': 2**64-1, 'xp_growth': '1.1'})
        self.assertTrue(balance.prestige_eligible([0, 1], 1))
        self.assertFalse(balance.prestige_eligible([0, 1], 2))
        self.assertEqual(balance.config['season'], 'test-season')
        self.assertEqual(balance.config['anti_cheat'], 'required')

    def test_singleplayer_example_and_server_defaults_match(self):
        root = Path(__file__).resolve().parents[1]
        ini = configparser.ConfigParser()
        ini.read(root/'Progression/fixtures/singleplayer.ini')
        from_ini = {'RERIFF_BALANCE_'+key.upper(): value for key, value in ini['balance'].items()}
        example = {}
        for raw in (root/'docker/server.env.example').read_text().splitlines():
            if raw.startswith('RERIFF_BALANCE_'):
                name, value = raw.split('=', 1)
                example[name] = value.strip().strip("'\"")
        for name, value in from_ini.items():
            self.assertEqual(example[name], value, name)
        self.assertEqual(Balance.from_env(example).config, self.balance.config)
        self.assertEqual(Balance.from_env(from_ini).config, self.balance.config)

    def test_invalid_environment_is_rejected_instead_of_using_defaults(self):
        cases = [('COIN_REWARD_RATE', 'NaN'), ('COIN_REWARD_RATE', ''), ('COIN_REWARD_RATE', '-1'),
                 ('CREATOR_REWARD_RATE', 'infinity'), ('PRESTIGE_MULTIPLIER_CAP', '.5'),
                 ('DISCOUNT_MODE', 'typo'), ('DISCOUNT_START_LEVEL', '1.5'),
                 ('DISCOUNT_START', '.6'), ('DISCOUNT_INCREASE', '-.1'), ('DISCOUNT_INTERVAL', '0'),
                 ('DISCOUNT_MAXIMUM', '1'), ('MAIN_LEVEL_CAP', '39'), ('MAIN_LEVEL_CAP', '40.5'),
                 ('MAIN_LEVEL_XP_STEP', str(2**64)), ('MAIN_LEVEL_XP_GROWTH', '.9'),
                 ('CAR_LEVELS', '{"5":10,"5":20}'), ('CAR_LEVELS', '{"22":1}'),
                 ('REQUIRED_ACHIEVEMENTS', '[]'), ('REQUIRED_ACHIEVEMENTS', '[0,0]'),
                 ('MAXIMUM_PRESTIGE_COUNT', '-1'), ('DISCOUNT_INTERVALL', '5')]
        for name, value in cases:
            with self.subTest(name=name, value=value), self.assertRaises(PolicyError):
                Balance.from_env({'RERIFF_BALANCE_'+name: value})

    def test_zero_multiplier_cap_is_uncapped_and_revisions_are_canonical(self):
        balance = Balance.from_env({'RERIFF_BALANCE_PRESTIGE_MULTIPLIER_CAP': '0.0',
                                    'RERIFF_BALANCE_DISCOUNT_START': '0.200'})
        self.assertEqual(balance.revision, self.balance.revision)
        self.assertEqual(balance.coin_reward(1, 100), 101)
        changed = Balance.from_env({'RERIFF_BALANCE_CREATOR_REWARD_RATE': '2'})
        self.assertNotEqual(changed.revision, balance.revision)

    def test_overflows_are_bounded_and_boolean_or_negative_rewards_refused(self):
        self.assertEqual(self.balance.coin_reward(2147483647, 2147483647), 2147483647)
        for bad in [-1, True, 1.5, '100']:
            with self.assertRaises(PolicyError): self.balance.coin_reward(bad, 0)

    def test_prestige_requires_the_full_native_profile_list(self):
        self.assertFalse(self.balance.prestige_eligible(list(range(38)), 0))
        self.assertTrue(self.balance.prestige_eligible(set(range(39)), 0))
        self.assertTrue(self.balance.prestige_eligible(set(range(39)) - {18}, 0))
        for missing in set(range(39)) - {18}:
            self.assertFalse(self.balance.prestige_eligible(set(range(39)) - {missing}, 0))
        config = self.balance.config
        config['prestige'] = {'required_achievements': [0, 1], 'maximum_count': 2}
        balance = Balance(config)
        self.assertTrue(balance.prestige_eligible([0, 1], 1))
        self.assertFalse(balance.prestige_eligible([0, 1], 2))

    def test_revision_is_canonical_and_does_not_mutate_existing_settings(self):
        config = self.balance.config
        config['xp']['main_rate'] = 1.0
        self.assertEqual(Balance(config).revision, self.balance.revision)
        changed = self.balance.config
        changed['coins']['base_rate'] = '2'
        self.assertNotEqual(Balance(changed).revision, self.balance.revision)
        self.assertEqual(self.balance.coin_reward(10, 0), 10)

    def test_invalid_settings_fail_closed(self):
        cases = [(['xp', 'main_rate'], 'NaN'), (['coins', 'base_rate'], -1), (['coins', 'multiplier_cap'], 0),
                 (['discounts', 'maximum'], '1'), (['discounts', 'diminishing_interval'], 0),
                 (['discounts', 'milestones'], [{'level': 20, 'fraction': '.2'}, {'level': 10, 'fraction': '.1'}]),
                 (['unlocks', 'car_levels'], {'22': 1}), (['prestige', 'required_achievements'], [[0]]),
                 (['prestige', 'required_achievements'], [0, 0]), (['prestige', 'maximum_count'], True),
                 (['schema'], True), (['season'], '../other-server'), (['anti_cheat'], 'trust-client')]
        for path, value in cases:
            config = self.balance.config
            target = config
            for key in path[:-1]: target = target[key]
            target[path[-1]] = value
            with self.subTest(path=path, value=value), self.assertRaises(PolicyError): Balance(config)

    def test_endpoint_has_identity_and_no_false_enforcement_claims(self):
        with tempfile.TemporaryDirectory() as temp:
            server_id = str(uuid.uuid4())
            app = Flask(__name__)
            register_endpoint(app, temp, server_id)
            client = app.test_client()
            response = client.get('/reriff/balance')
            self.assertEqual(response.json['serverId'], server_id)
            self.assertEqual(response.json['balanceRevision'], self.balance.revision)
            self.assertFalse(response.json['capabilities']['progressionEnforced'])
            self.assertFalse(response.json['capabilities']['antiCheatVerified'])
            self.assertEqual(response.headers['Cache-Control'], 'no-store')
            self.assertEqual(client.post('/reriff/balance', json={'coins': 100000}).status_code, 405)
            self.assertFalse(list(Path(temp).iterdir()))
            path = Path(temp)/'balance.json'
            config = self.balance.config
            config['coins']['base_rate'] = '2'
            path.write_text(json.dumps(config))
            self.assertEqual(client.get('/reriff/balance').json['balanceRevision'], response.json['balanceRevision'])
            path.write_text('broken')
            self.assertEqual(client.get('/reriff/balance').status_code, 200)

    def test_endpoint_uses_environment_at_startup_and_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            app = Flask('custom_balance')
            with patch.dict(os.environ, {'RERIFF_BALANCE_COIN_REWARD_RATE': '2'}):
                register_endpoint(app, temp, str(uuid.uuid4()))
            response = app.test_client().get('/reriff/balance')
            self.assertEqual(response.json['balance']['coins']['base_rate'], '2')
            self.assertNotEqual(response.json['balanceRevision'], self.balance.revision)
            invalid = Flask('invalid_balance')
            with patch.dict(os.environ, {'RERIFF_BALANCE_COIN_REWARD_RATE': 'broken'}):
                register_endpoint(invalid, temp, str(uuid.uuid4()))
            error = invalid.test_client().get('/reriff/balance')
            self.assertEqual(error.status_code, 503)
            self.assertNotIn(temp, error.get_data(as_text=True))
            self.assertFalse(list(Path(temp).iterdir()))

    def test_ambiguous_or_oversized_policy_does_not_fall_back(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/'balance.json'
            path.write_text('{"schema":1,"schema":1}')
            with self.assertRaisesRegex(PolicyError, 'Duplicate'): Balance.load(path)
            path.write_bytes(b' '*65537)
            with self.assertRaisesRegex(PolicyError, '64 KiB'): Balance.load(path)


if __name__ == '__main__':
    unittest.main()
