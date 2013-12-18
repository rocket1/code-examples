package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class Inspection {

    protected int _inspID = 0;
    public int getInspID() { return _inspID; }
    public void setInspID( final int val ) { _inspID = val; }
    
    protected Recipe _recipe = null;
    public Recipe getRecipe() { return _recipe; }
    public void setRecipe( final Recipe val ) { _recipe = val; }

    protected String _serial = null;
    public String getSerial() { return _serial; }
    public void setSerial( final String val ) { _serial = val; }

    protected int _run = Anet.BAD_VAL;
    public int getRun() { return _run; }
    public void setRun( final int val ) { _run = val; }

    protected String _lotID = "0";
    public String getLotID() { return _lotID; }
    public void setLotID( final String val ) { _lotID = val; }

    protected long _timeOfInsp = Anet.BAD_VAL;
    public long getTimeOfInsp() { return _timeOfInsp; }
    public void setTimeOfInsp( final long val ) { _timeOfInsp = val; }

    protected long _cycleTime = Anet.BAD_VAL;
    public long getCycleTime() { return _cycleTime; }
    public void setCycleTime( final long val ) { _cycleTime = val; }

    protected String _operator = null;
    public String getOperator() { return _operator; }
    public void setOperator( final String val ) { _operator = val; }

    protected String _tool = null;
    public String getTool() { return _tool; }
    public void setTool( final String val ) { _tool = val; }

    protected String _line = null;
    public String getLine() { return _line; }
    public void setLine( final String val ) { _line = val; }

    protected String _centiTimestamp = null;
    public String getCentiTimestamp() { return _centiTimestamp; }
    public void setCentiTimestamp( final String val ) { _centiTimestamp = val; }

    protected int _lane = Anet.BAD_VAL;
    public int getLane() { return _lane; }
    public void setLane( final int val ) { _lane = val; }

    protected Boolean _boardPassed = false;
    public Boolean getBoardPassed() { return _boardPassed; }
    public void setBoardPassed( Boolean val ) { _boardPassed = val; }

    protected int _numGroupsFailed = Anet.BAD_VAL;
    public int getNumGroupsFailed() { return _numGroupsFailed; }
    public void setNumGroupsFailed( final int val ) { _numGroupsFailed = val; }

 //   public Pass getPass( final int pass );

 //   public Panel getPanel( final int panel );

    public ArrayList<Integer> getResultByAlgoNumber( final int algoNumber, final int level, final int resNdx ) throws AnetException {
        Algo algo = _recipe.getAlgoByAlgoNumber( algoNumber, level );
        return getResultByAlgo( algo, resNdx);
    }

    public ArrayList<Integer> getResultByAlgo( final Algo algo, final int resNdx ) throws AnetException {

    	try {

    	    String sql = new String();
    	    
    	    if ( algo.getLevel() == Anet.LEVEL2 ) {
    		
        		sql = "SELECT result_vector"
        		    + " FROM group_results AS gr, group_meta AS gm"
        		    + " WHERE gr.group_id=gm.group_id AND gm.algo_number='"
        		    + algo.getAlgoNumber()
        		    + "'";
    	    }
    	    else if ( algo.getLevel() == Anet.LEVEL1 ) {

        		sql = "SELECT result_vector"
        		    + " FROM item_results AS ir, item_meta AS im"
        		    + " WHERE ir.item_id=im.item_id"
        		    + " AND im.algo_number='"
        		    + algo.getAlgoNumber()
        		    + "'";
    	    }
    	    else {
    	       throw new AnetException( "Bad algo level specified." );
    	    }
    	    
    	    sql += " AND insp_id='" + _inspID + "'";
    	    
    	    ResultSet rs = Anet.get().sqlQuery(sql);
    	    ArrayList<Integer> ret = new ArrayList();
    	    
    	    while ( rs.next() ) {
    		
        		ResultChain resultChain = ResultChain.dbStrToResultChain( rs.getString("result_vector") );
        		Result resultZero = resultChain.getResult( 0 );
        		ret.add( Integer.valueOf( resultZero.getEntry(resNdx).getVal() ) );
    	    }
    	    
    	    return ret;
    	}
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}
    	catch ( Exception e ) {
    	   e.printStackTrace();
    	       
    	    throw new AnetException(e.getMessage());
    	}
    }

    //public ArrayList<InspGroup> getInspGroups( Boolean failedOnly ) {
    
    
   // }

 //   public ArrayList<InspGroup> getInspGroups( final int panel, Boolean failedOnly );

   // public ArrayList<Panel> getPanels() {
    
    
   // }

    public Repair getRepair() throws AnetException {
    
        String sql = Repair.getRepairSelectStmt()
            + " WHERE insp_id='" + _inspID + "'";
            
        ResultSet rs = Anet.get().sqlQuery(sql);
	    
        try {
            if ( ! rs.next() ) {
                throw new AnetException( "The repair information was not available." );
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}

        return Repair.createRepairFromRS( rs );
    }
 
    public ArrayList<InspItem> getInspItemsByAlgoName( final String algoName ) throws AnetException {
    
        final String sql = InspItem.getInspItemSelectStmt()
            + " AND im.algo_id=am.algo_id"
            + " AND am.algo_type_id=at.algo_type_id"
            + " AND at.algo_type='" + algoName + "'"
            + " AND ir.insp_id='" + _inspID + "'"; 

        try {
        
            ResultSet rs = Anet.get().sqlQuery(sql);
            ArrayList<InspItem> inspItems = new ArrayList();
            
            while ( rs.next() ) {
                inspItems.add( InspItem.createInspItemFromRS(rs) );
            }
            
            return inspItems;
        }
        catch ( SQLException e ) {
            throw new AnetException(e);
        }
    }    

    static public Inspection createInspectionFromRS( ResultSet rs ) throws AnetException {
        
        try {
    
            Recipe recipe = Recipe.createRecipeFromRS( rs );
    	    
    		Inspection insp = new Inspection();

    		insp.setRecipe( recipe );
    		insp.setInspID( rs.getInt("insp_id") );
            insp.setSerial( rs.getString("serial") );
            insp.setRun( rs.getInt("run") );
            insp.setLotID( rs.getString("lot_id") );
            insp.setTimeOfInsp( rs.getLong("time_of_insp") );
            insp.setCycleTime( rs.getLong("cycle_time") );
            insp.setOperator( rs.getString("operator") );
            insp.setTool( rs.getString("machine") );
            insp.setLine( rs.getString("line") );
            insp.setCentiTimestamp( rs.getString("centi_timestamp") );
            insp.setLane( rs.getInt("lane") );
            insp.setBoardPassed( rs.getBoolean("passed") );
            insp.setNumGroupsFailed( rs.getInt("ngroup_fails") );

            return insp;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
 
    static public final String getInspectionSelectStmt() {
    
        return "SELECT"
            + " ihdr.recipe_id,"
            + " recipe_name,"
            + " recipe_mod,"
            + " insp_id,"
            + " serial,"
            + " run,"
            + " lot_id,"
            + " time_of_insp,"
            + " centi_timestamp,"
            + " cycle_time,"
            + " operator,"
            + " machine,"
            + " line,"
            + " lane,"
            + " passed,"
            + " ngroup_fails"
            + " FROM insp_hdr AS ihdr,"
            + "      recipe_meta AS rm"
            + " WHERE ihdr.recipe_id=rm.recipe_id";
    }
 
    public String toString() {
        
        return "Recipe: " + _recipe.getName() + " (modified: " + _recipe.getModDate() + ")\n"
            + "Serial: " + _serial + "\n"
            + "Run: " + _run + "\n"
            + "Lot ID: " + _lotID + "\n"
            + "Time of Inspection: " + _timeOfInsp + "\n"
            + "Cycle Time: " + _cycleTime + "\n"
            + "Operator: " + _operator + "\n"
            + "Tool: " + _tool + "\n"
            + "Line: " + _line + "\n"
            + "Lane: " + _lane + "\n"
            + "Board Passed: " + _boardPassed + "\n"
            + "Num Groups Failed: " + _numGroupsFailed;
    }
 }
