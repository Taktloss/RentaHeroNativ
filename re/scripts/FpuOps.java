// SPDX-License-Identifier: GPL-3.0-or-later
// Listet FPU-Befehle (F*) und CALLs einer Funktion – kompakt, um ftol-Operanden zuzuordnen.
// Argumente: <addr hex> [...]   Ausgabe: "> <addr> | <befehl>"
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class FpuOps extends GhidraScript {

    @Override
    protected void run() throws Exception {
        for (String a : getScriptArgs()) {
            Function f = getFunctionContaining(toAddr(Long.parseLong(a, 16)));
            if (f == null) { println("> keine Funktion bei " + a); continue; }
            println("> == " + f.getName() + " " + f.getEntryPoint());
            for (Instruction in : currentProgram.getListing().getInstructions(f.getBody(), true)) {
                String m = in.getMnemonicString().toUpperCase();
                if (m.startsWith("F") || m.equals("CALL")) println("> " + in.getAddress() + " | " + in);
            }
        }
    }
}
