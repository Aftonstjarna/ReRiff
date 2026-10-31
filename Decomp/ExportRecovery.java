// Export targeted native decompilation and call references for recovery research.
// @category ReRiff
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportRecovery extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("output-directory address...");
        if (!"2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2"
                .equalsIgnoreCase(currentProgram.getExecutableSHA256()))
            throw new IllegalStateException("Unsupported original Game.exe hash");
        Path output = Path.of(args[0]);
        Files.createDirectories(output);
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram))
                throw new IllegalStateException(decompiler.getLastMessage());
            for (int i = 1; i < args.length; ++i) {
                monitor.checkCancelled();
                Address address = toAddr(args[i]);
                Function function = getFunctionAt(address);
                if (function == null) {
                    disassemble(address);
                    function = createFunction(address, null);
                }
                if (function == null) throw new IllegalStateException("No function at " + address);
                try (PrintWriter writer = new PrintWriter(output.resolve(args[i] + ".txt").toFile())) {
                    writer.println("Research output only; not verified reconstructed source.");
                    writer.println("Program: " + currentProgram.getName());
                    writer.println("SHA256: " + currentProgram.getExecutableSHA256());
                    writer.println("Language: " + currentProgram.getLanguageID());
                    writer.println("Function: " + function.getName() + " " + function.getEntryPoint());
                    writer.println("Body: " + function.getBody());
                    writer.println("Callers:");
                    for (Function caller : function.getCallingFunctions(monitor))
                        writer.println(caller.getEntryPoint() + " " + caller.getName());
                    writer.println("Callees:");
                    for (Function callee : function.getCalledFunctions(monitor))
                        writer.println(callee.getEntryPoint() + " " + callee.getName());
                    var result = decompiler.decompileFunction(function, 120, monitor);
                    if (!result.decompileCompleted())
                        throw new IllegalStateException(result.getErrorMessage());
                    writer.println(result.getDecompiledFunction().getC());
                }
            }
        } finally { decompiler.dispose(); }
    }
}
