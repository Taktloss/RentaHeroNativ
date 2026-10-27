// Exportiert kompakte Analysetabellen aus dem aktuellen Ghidra-Programm.
// Ausgabeverzeichnis: erstes Skript-Argument (Standard: Projektverzeichnis).
//
//   functions.tsv  addr, name, size, callers, callees, flags, apis, strings
//   vtables.tsv    addr, anzahl aufeinanderfolgender Funktionszeiger, ziele
//
// Es wird bewusst keine Disassembly exportiert; einzelne Funktionen werden
// gezielt mit DecompileFunction.java betrachtet.
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.StringDataInstance;

import java.io.File;
import java.io.PrintWriter;
import java.util.*;

public class ExportSummary extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();

        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();
        ReferenceManager rm = currentProgram.getReferenceManager();

        try (PrintWriter out = new PrintWriter(new File(outDir, "functions.tsv"), "UTF-8")) {
            out.println("addr\tname\tsize\tcallers\tcallees\tflags\tapis\tstrings");
            for (Function f : fm.getFunctions(true)) {
                if (f.isExternal()) {
                    continue;
                }
                Set<Function> callers = f.getCallingFunctions(monitor);
                Set<Function> callees = f.getCalledFunctions(monitor);
                TreeSet<String> apis = new TreeSet<>();
                for (Function c : callees) {
                    Function t = c.isThunk() ? c.getThunkedFunction(true) : c;
                    if (t != null && t.isExternal()) {
                        apis.add(t.getName());
                    }
                }
                LinkedHashSet<String> strs = new LinkedHashSet<>();
                for (Instruction ins : listing.getInstructions(f.getBody(), true)) {
                    for (Reference r : ins.getReferencesFrom()) {
                        Data d = listing.getDataAt(r.getToAddress());
                        if (d != null && d.hasStringValue() && strs.size() < 6) {
                            String s = StringDataInstance.getStringDataInstance(d).getStringValue();
                            if (s != null) {
                                strs.add(s.replaceAll("[\\t\\r\\n]+", " ").trim());
                            }
                        }
                        // indirekte API-Aufrufe über die IAT (call dword ptr [IAT])
                        Symbol sym = currentProgram.getSymbolTable().getPrimarySymbol(r.getToAddress());
                        if (sym != null && sym.isExternalEntryPoint() == false
                                && sym.getSource() == SourceType.IMPORTED
                                && r.getReferenceType().isCall()) {
                            apis.add(sym.getName());
                        }
                    }
                }
                String flags = (f.isThunk() ? "T" : "") + (f.getSymbol().getSource() != SourceType.DEFAULT ? "N" : "");
                out.printf("%s\t%s\t%d\t%d\t%d\t%s\t%s\t%s%n",
                        f.getEntryPoint(), f.getName(), f.getBody().getNumAddresses(),
                        callers.size(), callees.size(), flags,
                        String.join(",", apis), String.join(" | ", strs));
            }
        }

        // vtable-Kandidaten: >= 3 aufeinanderfolgende Zeiger auf Funktionsanfänge in .rdata/.data
        try (PrintWriter out = new PrintWriter(new File(outDir, "vtables.tsv"), "UTF-8")) {
            out.println("addr\tcount\tfirst_targets");
            for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
                if (!(b.getName().equals(".rdata") || b.getName().equals(".data"))) {
                    continue;
                }
                Address a = b.getStart();
                while (a.compareTo(b.getEnd()) < 0) {
                    int n = 0;
                    List<String> targets = new ArrayList<>();
                    Address p = a;
                    while (p.add(3).compareTo(b.getEnd()) <= 0) {
                        long v = currentProgram.getMemory().getInt(p) & 0xffffffffL;
                        Function t = fm.getFunctionAt(currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(v));
                        if (t == null) {
                            break;
                        }
                        if (targets.size() < 4) {
                            targets.add(t.getName());
                        }
                        n++;
                        p = p.add(4);
                    }
                    if (n >= 3 && rm.getReferenceCountTo(a) > 0) {
                        out.printf("%s\t%d\t%s%n", a, n, String.join(",", targets));
                    }
                    a = n > 0 ? p : a.add(4);
                }
            }
        }
        println("ExportSummary: fertig -> " + outDir.getAbsolutePath());
    }
}
