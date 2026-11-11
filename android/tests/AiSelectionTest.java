// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

public final class AiSelectionTest {
    private static void require(boolean passed,String message){
        if(!passed)throw new AssertionError(message);
    }
    public static void main(String[] args){
        require(AiSelection.BASIC.equals(AiSelection.migrate(null,false,false)),"default");
        require(AiSelection.APEX_6M.equals(AiSelection.migrate(null,true,false)),"legacy 6M");
        require(AiSelection.APEX_10M.equals(AiSelection.migrate(null,false,true)),"legacy spatial");
        require(AiSelection.APEX_10M.equals(AiSelection.migrate(null,true,true)),"legacy collision");
        for(String input:new String[]{AiSelection.BASIC,AiSelection.APEX_6M,AiSelection.APEX_10M,AiSelection.AUDONIC,"invalid",""}){
            String selected=AiSelection.normalize(input);
            require(!(AiSelection.usesApex6M(selected)&&AiSelection.usesSpatialPreview(selected)),"two AI paths");
            require(selected.equals(AiSelection.migrate(selected,true,true)),"saved selection priority");
        }
        require(AiSelection.AUDONIC.equals(AiSelection.migrate("audonic",true,true)),"AUDONIC preference preserved");
        require(AiSelection.usesApex6M("apex6m"),"6M on");
        require(!AiSelection.usesSpatialPreview("apex6m"),"10M off on 6M");
        require(!AiSelection.usesApex6M("apex10m"),"6M off on 10M");
        require(AiSelection.usesSpatialPreview("apex10m"),"10M on");
        System.out.println("PASS: exclusive AI radio state, safe migration and 6M/10M routing");
    }
}
