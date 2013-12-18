package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class RepairGroup extends InspGroup {

    protected int _repairID = Anet.BAD_VAL;
    public void setRepairID( final int val ) { _repairID = val; }

    protected ArrayList<Integer> _repairCodes = new ArrayList();
    public ArrayList<Integer> getRepairCodes() { return _repairCodes; }
    public void setRepairCodes( final ArrayList<Integer> val ) { _repairCodes = val; }
    
    protected ArrayList<Integer> _suggestedCodes = new ArrayList();
    public ArrayList<Integer> getSuggestedCodes() { return _suggestedCodes; }
    public void setSuggestedCodes( final ArrayList<Integer> val ) { _suggestedCodes = val; }

    public Boolean hasRepairCode( final int repairCode ) {
        return _repairCodes.contains( repairCode );
    }
    
    public ArrayList<RepairItem> getRepairItems() throws AnetException {
    
        String sql = RepairItem.getRepairItemSelectStmt() 
            + " WHERE repair_id='" + _repairID + "'";

        ResultSet rs = Anet.get().sqlQuery(sql);   
        ArrayList<RepairItem> retlist = new ArrayList();

        try {    
    	    while ( rs.next() ) {
                retlist.add( RepairItem.createRepairItemFromRS(rs) );
    	    }
        }
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}   

        return retlist;
    }
    
    static public String getRepairGroupSelectStmt() {
     
        return "SELECT"
            + " gm.ref_name,"
            + " gm.part_type,"
            + " gm.x_loc AS gm_x_loc,"
            + " gm.y_loc AS gm_y_loc,"
            + " gm.dx_loc AS gm_dx_loc,"
            + " gm.dy_loc AS gm_dy_loc,"
            + " gm.algo_number AS gm_algo_number,"
            + " gm.orient AS gm_orient,"
            + " gm.panel_y,"
            + " gm.pass,"
            + " gm.visible AS gm_visible,"
            + " g_rep.repair_id,"
            + " g_rep.group_id,"
            + " g_rep.repair_code,"
            + " g_rep.suggested_code"
            + " FROM group_repair AS g_rep, group_meta AS gm"
            + " WHERE gm.group_id=g_rep.group_id";
    }
  
    static public RepairGroup createRepairGroupFromRS( ResultSet rs ) throws AnetException {	
    
        try {
        
            RepairGroup repairGroup = new RepairGroup();
            
            repairGroup.setRefName( rs.getString("ref_name") );
            repairGroup.setXLoc( rs.getInt("gm_x_loc") );
            repairGroup.setYLoc( rs.getInt("gm_y_loc") );
            repairGroup.setDXLoc( rs.getInt("gm_dx_loc") );
            repairGroup.setDYLoc( rs.getInt("gm_dy_loc") );
            repairGroup.setAlgoNumber( rs.getInt("gm_algo_number") );
            repairGroup.setOrientation( rs.getInt("gm_orient") );
            repairGroup.setPanelNum( rs.getInt("panel_y") );
            repairGroup.setPassNum( rs.getInt("pass") );
            repairGroup.isVisible( rs.getBoolean("gm_visible") );
//            repairGroup.setDecision( rs.getInt("decision") );
            repairGroup.setRepairID( rs.getInt("repair_id") );
            repairGroup.setGroupID( rs.getInt("group_id") );
            repairGroup.setRepairCodes( AnetUtil.str2IntArray( rs.getString("repair_code") ) );
            repairGroup.setSuggestedCodes( AnetUtil.str2IntArray( rs.getString("suggested_code") ) );
                     
            return repairGroup;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
    
    public String toString() {
    
        String str = "Ref Name: " + _refName + " (algo:" + _algoNumber +")";
         
        for ( Integer code : _repairCodes ) {
        
            try { 
                str += Repair.getRepairLabel( code ) + " ";
            }
            catch ( AnetException e ) {
                str += code + " ";
            }
        }
        
        return str;
    }
}
