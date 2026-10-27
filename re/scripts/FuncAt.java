// Gibt für jede Adresse die enthaltende Funktion aus (Einstieg, Name, Größe).
// Argumente: <addr hex>...
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

public class FuncAt extends GhidraScript {

    @Override
    protected void run() throws Exception {
        for (String a : getScriptArgs()) {
            Function f = getFunctionContaining(toAddr(Long.parseLong(a, 16)));
            println(a + " -> " + (f == null ? "keine Funktion"
                    : f.getEntryPoint() + " " + f.getName() + " " + f.getBody().getNumAddresses()));
        }
    }
}
