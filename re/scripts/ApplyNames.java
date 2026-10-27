// Überträgt Namen aus einer TSV-Datei ins Ghidra-Programm.
// Format pro Zeile: <addr hex>\t<name>\t<f|d>   (f = Funktion, d = Daten/Label)
// '#'-Zeilen und Leerzeilen werden ignoriert.
// Argumente: <names.tsv>
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

import java.nio.file.Files;
import java.nio.file.Paths;

public class ApplyNames extends GhidraScript {

    @Override
    protected void run() throws Exception {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
            line = line.strip();
            if (line.isEmpty() || line.startsWith("#")) {
                continue;
            }
            String[] p = line.split("\t");
            Address a = toAddr(Long.parseLong(p[0], 16));
            if (p[2].equals("f")) {
                Function f = getFunctionAt(a);
                if (f == null) {
                    f = createFunction(a, null);
                }
                if (f == null) {
                    println("keine Funktion bei " + p[0]);
                    continue;
                }
                f.setName(p[1], SourceType.USER_DEFINED);
            } else {
                createLabel(a, p[1], true, SourceType.USER_DEFINED);
            }
            n++;
        }
        println("ApplyNames: " + n + " Namen gesetzt");
    }
}
