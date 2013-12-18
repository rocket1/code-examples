package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class Group {

    protected int _groupID = Anet.BAD_VAL;
    public int getGroupID() { return _groupID; }
    public void setGroupID( final int val ) { _groupID = val; }

    protected String _refName = null;
    public String getRefName() { return _refName; }
    public void setRefName( final String val ) { _refName = val; }
    
    protected int _xLoc = Anet.BAD_VAL;
    public int getXLoc() { return _xLoc; }
    public void setXLoc( final int val ) { _xLoc = val; }

    protected int _yLoc = Anet.BAD_VAL;
    public int getYLoc() { return _yLoc; }
    public void setYLoc( final int val ) { _yLoc = val; }

    protected int _dxLoc = Anet.BAD_VAL;
    public int getDXLoc() { return _dxLoc; }
    public void setDXLoc( final int val ) { _dxLoc = val; }

    protected int _dyLoc = Anet.BAD_VAL;
    public int getDYLoc() { return _dyLoc; }
    public void setDYLoc( final int val ) { _dyLoc = val; }

    protected int _algoNumber = Anet.BAD_VAL;
    public int getAlgoNumber() { return _algoNumber; }
    public void setAlgoNumber( final int val ) { _algoNumber = val; }

    protected int _orientation = Anet.BAD_VAL;
    public int getOrientation() { return _orientation; }
    public void setOrientation( final int val ) { _orientation = val; }

    protected int _panelNum = Anet.BAD_VAL;
    public int getPanelNum() { return _panelNum; }
    public void setPanelNum( final int val ) { _panelNum = val; }

    protected int _passNum = Anet.BAD_VAL;
    public int getPassNum() { return _passNum; }
    public void setPassNum( final int val ) { _passNum = val; }

    protected Boolean _isVisible = false;
    public Boolean isVisible() { return _isVisible; }
    public void isVisible( Boolean val ) { _isVisible = val; }

 //   public Item getItem( final int pin );

 //   public ArrayList<Item> getItems( Boolean failedOnly );
 
    static public final String getGroupSelectStmt() {
    
        return "SELECT"
            + " gm.ref_name,"
            + " gm.x_loc AS gm_x_loc,"
            + " gm.y_loc AS gm_y_loc,"
            + " gm.dx_loc AS gm_dx_loc,"
            + " gm.dy_loc AS gm_dy_loc,"
            + " gm.algo_number AS gm_algo_number,"
            + " gm.orient AS gm_orient,"
            + " gm.panel_y,"
            + " gm.pass,"
            + " gm.visible AS gm_visible"
            + " FROM group_meta AS gm";
    }
 
    static public Group createGroupFromRS( ResultSet rs ) throws AnetException {
    
        try {

            Group group = new Group();

            group.setRefName( rs.getString("ref_name") );
            group.setXLoc( rs.getInt("gm_x_loc") );
            group.setYLoc( rs.getInt("gm_y_loc") );
            group.setDXLoc( rs.getInt("gm_dx_loc") );
            group.setDYLoc( rs.getInt("gm_dy_loc") );
            group.setAlgoNumber( rs.getInt("gm_algo_number") );
            group.setOrientation( rs.getInt("gm_orient") );
            group.setPanelNum( rs.getInt("panel_y") );
            group.setPassNum( rs.getInt("pass") );
            group.isVisible( rs.getBoolean("gm_visible") );
            
            return group;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
    
    public String toString() {
        return "Ref Name: " + getRefName();
    }
}
