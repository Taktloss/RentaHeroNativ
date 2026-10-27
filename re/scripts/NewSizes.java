// Findet "operator new(size)"-Aufrufe und den direkt folgenden Konstruktoraufruf.
// Ausgabe nach <outdir>/news.tsv: size, call_site, caller, ctor
// Argumente: <outdir> <operator_new_addr_hex>
//@category RentAHero

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;

import java.io.File;
import java.io.PrintWriter;

public class NewSizes extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        Address opNew = toAddr(Long.parseLong(args[1], 16));
        Listing listing = currentProgram.getListing();

        try (PrintWriter out = new PrintWriter(new File(args[0], "news.tsv"), "UTF-8")) {
            out.println("size\tcall_site\tcaller\tctor");
            for (Reference r : getReferencesTo(opNew)) {
                if (!r.getReferenceType().isCall()) {
                    continue;
                }
                Instruction call = listing.getInstructionAt(r.getFromAddress());
                // Größe: letztes "PUSH imm" vor dem Aufruf (max. 6 Befehle zurück)
                String size = "?";
                Instruction p = call.getPrevious();
                for (int i = 0; i < 6 && p != null; i++, p = p.getPrevious()) {
                    if (p.getMnemonicString().equals("PUSH") && p.getScalar(0) != null) {
                        size = "0x" + Long.toHexString(p.getScalar(0).getUnsignedValue());
                        break;
                    }
                }
                // Konstruktor: erster CALL nach dem new (max. 16 Befehle vorwärts)
                String ctor = "?";
                Instruction n = call.getNext();
                for (int i = 0; i < 16 && n != null; i++, n = n.getNext()) {
                    if (n.getFlowType().isCall()) {
                        Address[] flows = n.getFlows();
                        if (flows.length > 0) {
                            ctor = flows[0].toString();
                        }
                        break;
                    }
                }
                Function caller = getFunctionContaining(call.getAddress());
                out.printf("%s\t%s\t%s\t%s%n", size, call.getAddress(),
                        caller == null ? "?" : caller.getEntryPoint().toString(), ctor);
            }
        }
        println("NewSizes: fertig");
    }
}
