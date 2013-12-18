package com.visionpro.anet;

import java.sql.*;

public class InspGroup extends Group {

    protected int _decision = Anet.BAD_VAL;
    public int getDecision() { return _decision; }
    public void setDecision( final int val ) { _decision = val; }
    
    protected Result _result;
    public Result getResult() { return _result; }
    public void setResult( final Result val ) { _result = val; }
    
    static public InspGroup createInspGroupFromRS( ResultSet rs ) throws AnetException {
    
        try {

            InspGroup inspGroup = new InspGroup();

            inspGroup.setRefName( rs.getString("ref_name") );
            inspGroup.setXLoc( rs.getInt("gm_x_loc") );
            inspGroup.setYLoc( rs.getInt("gm_y_loc") );
            inspGroup.setDXLoc( rs.getInt("gm_dx_loc") );
            inspGroup.setDYLoc( rs.getInt("gm_dy_loc") );
            inspGroup.setAlgoNumber( rs.getInt("gm_algo_number") );
            inspGroup.setOrientation( rs.getInt("gm_orient") );
            inspGroup.setPanelNum( rs.getInt("panel_y") );
            inspGroup.setPassNum( rs.getInt("pass") );
            inspGroup.isVisible( rs.getBoolean("gm_visible") );
            inspGroup.setDecision( rs.getInt("decision") );
            
            return inspGroup;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
}
