// SPDX-License-Identifier: GPL-3.0-or-later
// Gibt die Befehle im Bereich [von, bis) aus – nur für kurze Abschnitte (Token sparen).
// Argumente: <von hex> <bis hex>
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;

public class Disasm extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long end = Long.parseLong(a[1], 16);
        Instruction i = getInstructionContaining(toAddr(Long.parseLong(a[0], 16)));
        while (i != null && i.getAddress().getOffset() < end) {
            println("> " + i.getAddress() + " " + i);
            i = i.getNext();
        }
    }
}
