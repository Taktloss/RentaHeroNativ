// SPDX-License-Identifier: GPL-3.0-or-later
// Kompakte Übersicht der Aufrufe einer Funktion: je CALL die Adresse, das Ziel und die
// Operanden der unmittelbar vorangehenden PUSH-Befehle (in Push-Reihenfolge, letzter = 1. Argument).
// Argumente: <addr hex> [...]   Ausgabe: "> <call-addr> <ziel> | push ..."
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

import java.util.ArrayList;
import java.util.List;

public class CallArgs extends GhidraScript {

    @Override
    protected void run() throws Exception {
        for (String a : getScriptArgs()) {
            Function f = getFunctionContaining(toAddr(Long.parseLong(a, 16)));
            if (f == null) {
                println("> " + a + " keine Funktion");
                continue;
            }
            println("> == " + f.getEntryPoint() + " " + f.getName());
            List<String> pushes = new ArrayList<>();
            for (Instruction in : currentProgram.getListing().getInstructions(f.getBody(), true)) {
                String m = in.getMnemonicString();
                if (m.equalsIgnoreCase("PUSH")) {
                    pushes.add(in.getDefaultOperandRepresentation(0));
                } else if (m.equalsIgnoreCase("CALL")) {
                    println("> " + in.getAddress() + " " + in.getDefaultOperandRepresentation(0) + " | "
                            + String.join(", ", pushes));
                    pushes.clear();
                } else if (m.toUpperCase().startsWith("J") || m.equalsIgnoreCase("RET")) {
                    pushes.clear();
                }
            }
        }
    }
}
