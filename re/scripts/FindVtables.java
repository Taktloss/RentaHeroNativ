// Findet vtables über Konstruktor-Muster: "mov dword ptr [reg(+0)], imm32", wobei imm32
// auf ein Array von Zeigern in .text zeigt. Ausgabe nach <outdir>/vtables2.tsv:
//   vtable, anzahl_slots, schreibende_funktionen, slot-ziele (alle)
// Argumente: <outdir> [max_text_addr_hex]  (Standard: 0044ea50 = Ende Spielcode)
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.scalar.Scalar;

import java.io.File;
import java.io.PrintWriter;
import java.util.*;

public class FindVtables extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args[0]);
        long codeEnd = args.length > 1 ? Long.parseLong(args[1], 16) : 0x44ea50L;
        Memory mem = currentProgram.getMemory();
        MemoryBlock text = mem.getBlock(".text");

        TreeMap<Long, TreeSet<String>> writers = new TreeMap<>();
        for (Instruction ins : currentProgram.getListing().getInstructions(true)) {
            if (ins.getAddress().getOffset() >= codeEnd) {
                break;
            }
            if (!ins.getMnemonicString().equals("MOV") || ins.getNumOperands() != 2) {
                continue;
            }
            int t0 = ins.getOperandType(0);
            if ((t0 & OperandType.DYNAMIC) == 0) {   // Ziel muss [reg] / [reg+disp] sein
                continue;
            }
            Scalar s = ins.getScalar(1);
            if (s == null) {
                continue;
            }
            long v = s.getUnsignedValue();
            MemoryBlock b = mem.getBlock(toAddr(v));
            if (b == null || !(b.getName().equals(".rdata") || b.getName().equals(".data"))) {
                continue;
            }
            Function f = getFunctionContaining(ins.getAddress());
            writers.computeIfAbsent(v, k -> new TreeSet<>())
                   .add(f == null ? ins.getAddress().toString() : f.getEntryPoint().toString());
        }

        try (PrintWriter out = new PrintWriter(new File(outDir, "vtables2.tsv"), "UTF-8")) {
            out.println("vtable\tslots\twriters\ttargets");
            for (Map.Entry<Long, TreeSet<String>> e : writers.entrySet()) {
                Address a = toAddr(e.getKey());
                List<String> targets = new ArrayList<>();
                while (true) {
                    long p = mem.getInt(a.add(targets.size() * 4L)) & 0xffffffffL;
                    if (!text.contains(toAddr(p))) {
                        break;
                    }
                    targets.add(String.format("%08x", p));
                    if (targets.size() > 200) {
                        break;
                    }
                }
                if (targets.isEmpty()) {
                    continue;
                }
                out.printf("%08x\t%d\t%s\t%s%n", e.getKey(), targets.size(),
                        String.join(",", e.getValue()), String.join(",", targets));
            }
        }
        println("FindVtables: fertig");
    }
}
