using System;
using System.Collections.Generic;
using System.IO;
namespace ReRiff {
    // Pure migration plan. Commit together with a verified native save snapshot;
    // never use this to mutate a live save piecemeal or guess an unknown layout.
    internal static class CarSlotMigration {
        internal sealed class Value {
            internal int Paint,Driven;internal bool Owned;internal ushort Packs;
            internal Value Copy(){return (Value)MemberwiseClone();}
        }
        internal sealed class Save {
            internal int Selected;internal Value[] Slots=new Value[22];
            internal readonly Dictionary<string,Value> Retired=new Dictionary<string,Value>(StringComparer.Ordinal);
            internal Save(){for(int i=0;i<Slots.Length;i++)Slots[i]=new Value();}
        }
        internal static Save Move(Save input,CarSlotRegistry.State before,CarSlotRegistry.State after){
            if(input==null||before==null||after==null||input.Selected<0||input.Selected>=22)throw new IOException("Car migration needs a known save layout");
            var output=new Save();output.Selected=input.Selected;
            foreach(var item in input.Retired)output.Retired[item.Key]=item.Value.Copy();
            for(int i=0;i<15;i++)output.Slots[i]=input.Slots[i].Copy();
            string selected=null,baseId=null;
            var occupied=new HashSet<int>();foreach(var car in before.Cars)if(car.Active&&!car.Overwrite){
                if(car.Slot<15||car.Slot>=22||!occupied.Add(car.Slot))throw new IOException("Ambiguous old car layout");
                output.Retired[car.Key]=input.Slots[car.Slot].Copy();if(input.Selected==car.Slot){selected=car.Key;baseId=car.Base;}
            }
            if(input.Selected>=15){if(selected==null)throw new IOException("Selected custom slot has no known identity");output.Selected=Array.IndexOf(CarSlotRegistry.Stock,baseId);}
            occupied.Clear();foreach(var car in after.Cars)if(car.Active&&!car.Overwrite){
                if(car.Slot<15||car.Slot>=22||!occupied.Add(car.Slot))throw new IOException("Ambiguous new car layout");
                Value value;if(output.Retired.TryGetValue(car.Key,out value))output.Slots[car.Slot]=value.Copy();
                // New identities start empty; native unlock rules initialize them.
                if(selected==car.Key)output.Selected=car.Slot;
            }
            return output;
        }
    }
}
