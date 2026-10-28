// SPDX-License-Identifier: GPL-3.0-or-later
// Wie CallArgs, löst aber Register-Operanden der PUSH-Befehle auf: zuletzt davor stehender
// MOV/XOR auf dieses Register (innerhalb der Funktion, rückwärts bis 400 Befehle).
// Argumente: <funktion hex> <ziel hex>:<anzahl args>...   Ausgabe: "> <call> <ziel> | arg1, arg2, ..." (1. Argument zuerst)
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class CallArgsResolved extends GhidraScript {

    private String resolve(Instruction from, String reg) {
        Instruction i = from.getPrevious();
        for (int n = 0; i != null && n < 400; ++n, i = i.getPrevious()) {
            String m = i.getMnemonicString();
            if (i.getNumOperands() < 1) continue;
            String dst = i.getDefaultOperandRepresentation(0);
            if (!dst.equals(reg)) continue;
            if (m.equals("XOR") && i.getDefaultOperandRepresentation(1).equals(reg)) return "0";
            if (m.equals("MOV") || m.equals("LEA")) {
                String src = i.getDefaultOperandRepresentation(1);
                if (src.matches("E[A-Z]X|E[SD]I|EBP")) return resolve(i, src);
                return src;
            }
            if (m.equals("PUSH") || m.equals("CMP") || m.equals("TEST") || m.equals("CALL")) continue;
            return reg + "?" + m;
        }
        return reg + "?";
    }

    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        Function f = getFunctionAt(toAddr(Long.parseLong(a[0], 16)));
        java.util.Map<Long, Integer> targets = new java.util.HashMap<>();
        for (int k = 1; k < a.length; ++k) {
            String[] tn = a[k].split(":");
            targets.put(Long.parseLong(tn[0], 16), Integer.parseInt(tn[1]));
        }
        for (Instruction i : currentProgram.getListing().getInstructions(f.getBody(), true)) {
            if (!i.getMnemonicString().equals("CALL")) continue;
            String t = i.getDefaultOperandRepresentation(0).toLowerCase();
            if (!t.startsWith("0x")) continue;
            Integer want = targets.get(Long.decode(t));
            if (want == null) continue;
            List<String> args = new ArrayList<>();
            Instruction p = i.getPrevious();
            String thisReg = null;
            for (int n = 0; p != null && n < 40 && args.size() < want; ++n, p = p.getPrevious()) {
                String m = p.getMnemonicString();
                if (m.equals("MOV") && p.getDefaultOperandRepresentation(0).equals("ECX") && thisReg == null)
                    thisReg = p.getDefaultOperandRepresentation(1);
                if (!m.equals("PUSH")) continue;
                String op = p.getDefaultOperandRepresentation(0);
                if (op.matches("E[A-Z]X|E[SD]I|EBP")) op = resolve(p, op);
                args.add(op);
            }
            println("> " + i.getAddress() + " " + t + " | this=" + thisReg + " | " + String.join(", ", args));
        }
    }
}
