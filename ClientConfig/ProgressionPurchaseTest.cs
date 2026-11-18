using System;
using System.IO;

namespace ReRiff {
    // Local development request only, never created by an installer or normal
    // client operation. The user explicitly authorized one purchase relock.
    internal static class ProgressionPurchaseTest {
#if RERIFF_LOCAL_PURCHASE_TEST
        private static bool inspected;
        internal static int Run(){
            if(inspected||!ProgressionCheckpoints.Ready||ProgressionNative.Bound==null)return 1;
            inspected=true;string request=ProgressionCheckpoints.PurchaseTestRequest;
            if(request==null||!File.Exists(request))return 1;
            bool changed=false;
            try {
                if(new FileInfo(request).Length>64||File.ReadAllText(request).Trim()!="relock-car-0-pack-1-v1")throw new IOException("Unrecognized purchase-test request");
                // Consume before any native mutation. Normal restart cannot
                // replay this request after the player buys the pack again.
                File.Move(request,request+".consumed");
                NativeReelBridge.Log(ProgressionNative.Bound.RelockForPurchaseTest(0,1,ref changed));return 1;
            }catch(InvalidOperationException error){NativeReelBridge.Log("Purchase-test request refused: "+error.Message);return changed?-1:1;}
            catch(Exception error){NativeReelBridge.Log("Purchase-test request failed; restart from the last committed checkpoint: "+error);return -1;}
        }
#else
        internal static int Run(){return 1;}
#endif
    }
}
