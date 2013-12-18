package com.visionpro.anet;

import java.sql.*;

public class InspItem extends Item {

    protected int _decision = Anet.BAD_VAL;
    public int getDecision() { return _decision; }
    public void setDecision( final int val ) { _decision = val; }
    
    protected ResultChain _resultChain;
    public ResultChain getResultChain() { return _resultChain; }
    public void setResultChain( final ResultChain val ) { _resultChain = val; }
    
    protected InspGroup _parentGroup;
    public InspGroup getParentGroup() { return _parentGroup; }
    public void setParentGroup( final InspGroup val ) { _parentGroup = val; }
    
    static public InspItem createInspItemFromRS( ResultSet rs ) throws AnetException {
    
        try {
        
       		InspItem inspItem = new InspItem();
            InspGroup inspGroup = InspGroup.createInspGroupFromRS( rs );
    
    		inspItem.setPin( rs.getInt("pin") );
            inspItem.setDecision( rs.getInt("decision") );
            inspItem.setXLoc( rs.getInt("im_x_loc") );
            inspItem.setYLoc( rs.getInt("im_y_loc") );
            inspItem.setDXLoc( rs.getInt("im_dx_loc") );
            inspItem.setDYLoc( rs.getInt("im_dy_loc") );
            inspItem.setAlgoNumber( rs.getInt("im_algo_number") );
            inspItem.setOrientation( rs.getInt("im_orient") );
            inspItem.isVisible( rs.getBoolean("im_visible") );
            
            inspItem.setParentGroup( inspGroup );
            inspItem.setResultChain( ResultChain.dbStrToResultChain( rs.getString("result_vector") ));
            
            return inspItem;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
    }
 
    static public final String getInspItemSelectStmt() {
    
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
            + " gm.visible AS gm_visible,"
            + " ir.result_vector,"
            + " ir.decision,"
            + " im.x_loc AS im_x_loc,"
            + " im.y_loc AS im_y_loc,"
            + " im.dx_loc AS im_dx_loc,"
            + " im.dy_loc AS im_dy_loc,"
            + " im.algo_number AS im_algo_number,"
            + " im.orient AS im_orient,"
            + " im.visible AS im_visible,"
            + " im.pin"
            + " FROM group_meta as gm,"
            + "      item_meta as im,"
            + "      item_results as ir,"
            + "      algo_meta as am,"
            + "      algo_type as at"
            + " WHERE im.group_id=gm.group_id"
            + " AND ir.item_id=im.item_id";     
    }
    
    public String toString() {
        
        return "Ref Name: " + _parentGroup.getRefName()
            + " Pin: " + _pin
            + " Decision: " + _decision
            + " Results: " + _resultChain.toString();
    }
}
