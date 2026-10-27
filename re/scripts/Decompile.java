// Dekompiliert einzelne Funktionen und schreibt sie nach <outdir>/<addr>.c.
// Argumente: <outdir> <addr> [<addr> ...]   (Adressen hex, z. B. 00449380)
// Zusätzlich: <outdir>/<addr>.xrefs mit Aufrufern und Aufgerufenen.
//@category RentAHero

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

import java.io.File;
import java.io.PrintWriter;

public class Decompile extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args[0]);
        outDir.mkdirs();

        DecompInterface ifc = new DecompInterface();
        ifc.openProgram(currentProgram);

        for (int i = 1; i < args.length; i++) {
            Address addr = toAddr(Long.parseLong(args[i], 16));
            Function f = getFunctionContaining(addr);
            if (f == null) {
                // z. B. Fensterprozeduren, die nur als Zeiger referenziert werden
                f = createFunction(addr, null);
                if (f != null) {
                    println("Funktion angelegt: " + f.getEntryPoint());
                }
            }
            if (f == null) {
                println("keine Funktion bei " + args[i]);
                continue;
            }
            String name = String.format("%08x", f.getEntryPoint().getOffset());
            DecompileResults res = ifc.decompileFunction(f, 120, monitor);
            try (PrintWriter out = new PrintWriter(new File(outDir, name + ".c"), "UTF-8")) {
                out.println("// " + f.getName() + " @ " + f.getEntryPoint());
                out.println(res.decompileCompleted() ? res.getDecompiledFunction().getC() : "// FEHLER: " + res.getErrorMessage());
            }
            try (PrintWriter out = new PrintWriter(new File(outDir, name + ".xrefs"), "UTF-8")) {
                for (Function c : f.getCallingFunctions(monitor)) {
                    out.println("caller\t" + c.getEntryPoint() + "\t" + c.getName());
                }
                for (Function c : f.getCalledFunctions(monitor)) {
                    out.println("callee\t" + c.getEntryPoint() + "\t" + c.getName());
                }
            }
            println("dekompiliert: " + name);
        }
        ifc.dispose();
    }
}
