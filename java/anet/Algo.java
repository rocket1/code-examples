package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class Algo {

    protected int _level = Anet.BAD_VAL;
    public int getLevel() { return _level; }
    public void setLevel( final int val ) { _level = val; }

    protected int _algoNumber = Anet.BAD_VAL;
    public int getAlgoNumber() { return _algoNumber; }
    public void setAlgoNumber( final int val ) { _algoNumber = val; }

    protected String _algoType = null;
    public String getAlgoType() { return _algoType; }
    public void setAlgoType( final String val ) { _algoType = val; }

    protected String _algoLabel = null;
    public String getAlgoLabel() { return _algoLabel; }
    public void setAlgoLabel( final String val ) { _algoLabel = val; }

    protected int _chainTo = Anet.BAD_VAL;
    public int getChainTo() { return _chainTo; }
    public void setChainTo( final int val ) { _chainTo = val; }

    protected Threshold _threshold = null;
    public Threshold getThreshold() { return _threshold; }
    public void setThreshold( final Threshold val ) { _threshold = val; }

    static public Algo createAlgoFromRS( ResultSet rs ) throws AnetException {
    
        try {
    
            Algo algo = new Algo();
            
            algo.setLevel( rs.getInt("algo_level") );
            algo.setAlgoNumber( rs.getInt("algo_number") );
            algo.setAlgoType( rs.getString("algo_type") );
            algo.setAlgoLabel( rs.getString("algo_label") );
            algo.setChainTo( rs.getInt("chain_to") );
            algo.setThreshold( Threshold.dbStrToThreshold( rs.getString("threshold") ));
            
            return algo;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    } 
 
    static public final String getAlgoSelectStmt() {
    
        return "SELECT"
            + " algo_number,"
            + " algo_type,"
            + " algo_label,"
            + " algo_level,"
            + " chain_to,"
            + " threshold"
            + " FROM algo_meta AS am,"
            + "      algo_type AS at"
            + " WHERE am.algo_type_id=at.algo_type_id";
    }
    
    public String toString() {
    
        return "Algo Label: " + _algoLabel
            + "Algo Number: " + _algoNumber
            + "Algo Type: " + _algoType
            + "Algo Level: " + _level
            + "Chain To: " + _chainTo;
    }
}
