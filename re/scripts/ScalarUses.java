// SPDX-License-Identifier: GPL-3.0-or-later
// Listet alle Befehle, die einen der angegebenen Werte als Operand-Konstante enthalten
// (Feldzugriffe [reg + off] lesend wie schreibend, LEA, PUSH, CMP …).
// Argumente: <wert hex> [<wert hex> …]   Ausgabe: "> <wert> <addr> <funktion> | <befehl>"
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.scalar.Scalar;

import java.util.HashSet;
import java.util.Set;

public class ScalarUses extends GhidraScript {

    @Override
    protected void run() throws Exception {
        Set<Long> want = new HashSet<>();
        for (String a : getScriptArgs()) want.add(Long.parseLong(a.replace("0x", ""), 16));
        for (Instruction in : currentProgram.getListing().getInstructions(true)) {
            for (int i = 0; i < in.getNumOperands(); ++i) {
                for (Object x : in.getOpObjects(i)) {
                    if (!(x instanceof Scalar)) continue;
                    long v = ((Scalar) x).getUnsignedValue();
                    if (!want.contains(v)) continue;
                    Function f = getFunctionContaining(in.getAddress());
                    println("> " + Long.toHexString(v) + " " + in.getAddress() + " "
                            + (f == null ? "?" : f.getName()) + " | " + in);
                }
            }
        }
    }
}
