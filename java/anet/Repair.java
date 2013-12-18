package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class Repair {
    
    static final public int FALSE_CALL          = 700;
    static final public int ACCEPTABLE          = 701;
    static final public int DEFECT              = 702;
    static final public int TRANSLATION         = 745;
    static final public int VOLUME              = 746;
    static final public int PASTE_BRIDGING      = 725;
    static final public int MISSING_PASTE       = 726;
    static final public int INSUFFICIENT_PASTE  = 727;
    static final public int EXCESS_PASTE        = 728;
    static final public int MISSING_PART        = 729;
    static final public int MISPLACED_PART      = 730;
    static final public int FLIPPED_COMPONENT   = 731;
    static final public int WRONG_COMPONENT     = 732;
    static final public int COPLANARITY         = 733;
    static final public int BRIDGING            = 703;
    static final public int MISSING             = 704;
    static final public int POLARITY            = 705;
    static final public int INSUFFICIENT_SOLDER = 706;
    static final public int EXCESS_SOLDER       = 707;
    static final public int TOMBSTONE           = 708;
    static final public int BILLBOARD           = 709;
    static final public int MISPLACED           = 710;
    static final public int LIFTED_LEAD         = 711;
    static final public int LIFTED_COMPONENT    = 712;
    static final public int WRONG_PART          = 713;
    static final public int FLIPPED_COMPONENT2  = 714; // huh? duplicate.
    static final public int UNSEATED            = 755;
    static final public int BENT_PIN            = 756;
    static final public int PTH_BRIDGE          = 757;
    static final public int BAD_CLINCH          = 758;
    static final public int PTH_OPEN_SOLDER     = 759;
    static final public int BENT_PIN2           = 761; // another duplicate.
    static final public int MISSING_PIN         = 762;
    static final public int LOW_PIN             = 763;
    static final public int FOREIGN_MATERIAL    = 764;

    protected static final Map<Integer, String> _repairLabelMap;
    
    static {

        Map<Integer, String> aMap = new HashMap();    
        
        aMap.put( FALSE_CALL,          "False Call" );
        aMap.put( ACCEPTABLE,          "Marginal" );
        aMap.put( DEFECT,              "Defect" );
        aMap.put( TRANSLATION,         "Translation" );
        aMap.put( VOLUME,              "Volume" );
        aMap.put( PASTE_BRIDGING,      "Paste Bridging" );
        aMap.put( MISSING_PASTE,       "Missing Paste" );
        aMap.put( INSUFFICIENT_PASTE,  "Insufficient Paste" );
        aMap.put( EXCESS_PASTE,        "Excess Paste" );
        aMap.put( MISSING_PART,        "Missing Part" );
        aMap.put( MISPLACED_PART,      "Misplaced Part" );
        aMap.put( FLIPPED_COMPONENT,   "Flipped Component" );
        aMap.put( WRONG_COMPONENT,     "Wrong Component" );
        aMap.put( COPLANARITY,         "Coplanarity" );
        aMap.put( BRIDGING,            "Bridging" );
        aMap.put( MISSING,             "Missing" );
        aMap.put( POLARITY,            "Polarity" );
        aMap.put( INSUFFICIENT_SOLDER, "Insufficient Solder" );
        aMap.put( EXCESS_SOLDER,       "Excess Solder" );
        aMap.put( TOMBSTONE,           "Tombstone" );
        aMap.put( BILLBOARD,           "Billboard" );
        aMap.put( MISPLACED,           "Misplaced" );
        aMap.put( LIFTED_LEAD,         "Lifted Lead" );
        aMap.put( LIFTED_COMPONENT,    "Lifted Component" );
        aMap.put( WRONG_PART,          "Wrong Part" );
        aMap.put( FLIPPED_COMPONENT,   "Flipped Component" );
        aMap.put( UNSEATED,            "Unseated" );
        aMap.put( BENT_PIN,            "Bent Pin" );
        aMap.put( PTH_BRIDGE,          "PTH Bridge" );
        aMap.put( BAD_CLINCH,          "Bad Clinch" );
        aMap.put( PTH_OPEN_SOLDER,     "PTH Open Solder" );
        aMap.put( BENT_PIN2,           "Bent Pin" );
        aMap.put( MISSING_PIN,         "Missing Pin" );
        aMap.put( LOW_PIN,             "Low Pin" );
        aMap.put( FOREIGN_MATERIAL,    "Foreign Material" );        
        
        _repairLabelMap = Collections.unmodifiableMap( aMap );
    }

    protected int _repairID = Anet.BAD_VAL;
    public void setRepairID( final int val ) { _repairID = val; }

    protected String _serial = null;
    public String getSerial() { return _serial; }
    public void setSerial( final String val ) { _serial = val; }

    protected String _timeOfRepair = null;
    public String getTimeOfRepair() { return _timeOfRepair; }
    public void setTimeOfRepair( final String val ) { _timeOfRepair = val; }

    protected String _operator = null;
    public String getOperator() { return _operator; }
    public void setOperator( final String val ) { _operator = val; }

    protected String _tool = null;
    public String getTool() { return _tool; }
    public void setTool( final String val ) { _tool = val; }
    
    public ArrayList<RepairGroup> getRepairGroups() throws AnetException {
    
        String sql = RepairGroup.getRepairGroupSelectStmt() 
            + " AND repair_id='" + _repairID + "'";

        ResultSet rs = Anet.get().sqlQuery(sql);   
        ArrayList<RepairGroup> retlist = new ArrayList();

        try {    
    	    while ( rs.next() ) {
                retlist.add( RepairGroup.createRepairGroupFromRS(rs) );
    	    }
        }
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}   

        return retlist;
    }
    
    static public String getRepairSelectStmt() {
     
        return "SELECT"
            + " repair_id,"
            + " serial,"
            + " machine,"
            + " operator,"
            + " time_of_repair"
            + " FROM repair_hdr";
    }
  
    static public Repair createRepairFromRS( ResultSet rs ) throws AnetException {	
    
        try {
        
            Repair repair = new Repair();
     
            repair.setRepairID( rs.getInt("repair_id") );
            repair.setSerial( rs.getString("serial") );
            repair.setTimeOfRepair( rs.getString("time_of_repair") );
            repair.setOperator( rs.getString("operator") );
            repair.setTool( rs.getString("machine") );
                     
            return repair;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
    
    public String toString() {
    
        return "Serial: " + _serial + "\n"
            + "Tool: " + _tool + "\n"
            + "Operator: " + _operator + "\n"
            + "Time of Repair: " + _timeOfRepair;
    }
 
    static public String getRepairLabel( final int repairCode ) throws AnetException {
        
        String label = _repairLabelMap.get( repairCode );
        
        if ( label == null ) {
            label = "Unknown Code (" + Integer.toString(repairCode) + ")";
//            throw new AnetException( "Repair label not found for code \"" + Integer.toString(repairCode) + "\"." );
        }
        
        return label;
    }
}
