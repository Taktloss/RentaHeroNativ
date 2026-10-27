// Listet alle Referenzen auf die angegebenen Adressen (Code oder Daten).
// Argumente: <addr> [<addr> ...]   (hex)
// Ausgabe pro Referenz: ziel, quelle, referenztyp (READ/WRITE/CALL/...), funktion
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class XRefs extends GhidraScript {

    @Override
    protected void run() throws Exception {
        for (String arg : getScriptArgs()) {
            Address target = toAddr(Long.parseLong(arg, 16));
            int n = 0;
            for (Reference r : getReferencesTo(target)) {
                Function f = getFunctionContaining(r.getFromAddress());
                println(String.format("%s <- %s %-12s %s", target, r.getFromAddress(),
                        r.getReferenceType(), f == null ? "?" : f.getName() + "@" + f.getEntryPoint()));
                n++;
            }
            println(target + ": " + n + " Referenzen");
        }
    }
}
