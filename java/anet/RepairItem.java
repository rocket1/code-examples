package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class RepairItem {

    protected int _repairID = Anet.BAD_VAL;
    public void setRepairID( final int val ) { _repairID = val; }

    protected int _itemID = Anet.BAD_VAL;
    public void setItemID( final int val ) { _itemID = val; }

    protected int _bridgingPin = Anet.BAD_VAL;
    public int getBridgingPin() { return _bridgingPin; }
    public void setBridgingPin( final int val ) { _bridgingPin = val; }

    protected ArrayList<Integer> _repairCodes = new ArrayList();
    public ArrayList<Integer> getRepairCodes() { return _repairCodes; }
    public void setRepairCodes( final ArrayList<Integer> val ) { _repairCodes = val; }
    
    protected ArrayList<Integer> _suggestedCodes = new ArrayList();
    public ArrayList<Integer> getSuggestedCodes() { return _suggestedCodes; }
    public void setSuggestedCodes( final ArrayList<Integer> val ) { _suggestedCodes = val; }
    
    public Boolean hasRepairCode( final int repairCode ) {
        return _repairCodes.contains( repairCode );
    }
    
    static public String getRepairItemSelectStmt() {
     
        return "SELECT"
            + " repair_id,"
            + " item_id,"
            + " bridging_pin,"
            + " repair_code,"
            + " suggested_code";
    }
  
    static public RepairItem createRepairItemFromRS( ResultSet rs ) throws AnetException {	
    
        try {
        
            RepairItem repairItem = new RepairItem();
     
            repairItem.setRepairID( rs.getInt("repair_id") );
            repairItem.setItemID( rs.getInt("item_id") );
            repairItem.setBridgingPin( rs.getInt("bridging_pin") );
            repairItem.setRepairCodes( AnetUtil.str2IntArray( rs.getString("repair_code") ) );
            repairItem.setSuggestedCodes( AnetUtil.str2IntArray( rs.getString("suggested_code") ) );
            
            return repairItem;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }

    
    public String toString() {
    
        return "";
    }
}
