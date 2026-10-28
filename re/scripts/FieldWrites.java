// SPDX-License-Identifier: GPL-3.0-or-later
// Listet Befehle, die in ein Feld [reg + off] schreiben (MOV/AND/OR/INC/... mit Speicherziel).
// Argumente: <offset hex> [max_text_addr_hex]   Ausgabe: "> <addr> <funktion> | <befehl>"
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.lang.Register;

public class FieldWrites extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        long off = Long.parseLong(args[0], 16);
        long max = args.length > 1 ? Long.parseLong(args[1], 16) : 0x0044ea50L;
        for (Instruction in : currentProgram.getListing().getInstructions(true)) {
            if (in.getAddress().getOffset() > max) break;
            String m = in.getMnemonicString().toUpperCase();
            if (!(m.equals("MOV") || m.equals("AND") || m.equals("OR") || m.equals("INC") || m.equals("DEC")
                    || m.equals("ADD") || m.equals("SUB") || m.startsWith("FST") || m.equals("XOR"))) continue;
            if (in.getNumOperands() < 1) continue;
            Object[] o = in.getOpObjects(0);
            String rep = in.getDefaultOperandRepresentation(0);
            if (!rep.contains("[")) continue;
            boolean hasReg = false, hasOff = false;
            for (Object x : o) {
                if (x instanceof Register && !((Register) x).getName().equals("ESP")
                        && !((Register) x).getName().equals("EBP")) hasReg = true;
                if (x instanceof Scalar && ((Scalar) x).getUnsignedValue() == off) hasOff = true;
            }
            if (!hasReg || !hasOff) continue;
            Function f = getFunctionContaining(in.getAddress());
            println("> " + in.getAddress() + " " + (f == null ? "?" : f.getName()) + " | " + in);
        }
    }
}
