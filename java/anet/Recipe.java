package com.visionpro.anet;

import java.util.*;
import java.sql.*;

public class Recipe {

    protected int _recipeID = Anet.BAD_VAL;
    public int getRecipeID() { return _recipeID; } 
    public void setRecipeID( final int val ) { _recipeID = val; }

    protected String _name = null;
    public String getName() { return _name; }
    public void setName( final String val ) { _name = val; }

    protected long _modDate = Anet.BAD_VAL;
    public long getModDate() { return _modDate; }
    public void setModDate( final long val ) { _modDate = val; }

    public ArrayList<Inspection> getInspections( final int limit ) throws AnetException {
        return getInspections( limit, 0, 0 );
    }

    public ArrayList<Inspection> getInspections( final int limit, final long start, final long end ) throws AnetException {

    	String between = "";
    	
    	if (start > 0 && end == 0) {
    	    between = " AND time_of_insp >= " + start + " ";
    	}
    	else if (start == 0 && end > 0) {
    	    between = " AND time_of_insp < " + end + " ";
    	}
    	else if ( (end < start) && (start > 0 && end > 0) ) {
    	    between = " AND time_of_insp BETWEEN " + start + " AND " + end
    		+ " ";
    	}
    	else {
    	   // No BETWEEN clause.
    	}

        String sql = Inspection.getInspectionSelectStmt()
    	    + " AND recipe_id=(SELECT recipe_id FROM recipe_meta WHERE recipe_name='" + _name + "'"
    	    + " AND recipe_mod='" + _modDate + "')"
    	    + between
    	    + " ORDER BY time_of_insp"
    	    + " DESC";
    	    
        if ( limit > 0 ) {
            sql += " LIMIT 0," + limit;
        }
    
	    ResultSet rs = Anet.get().sqlQuery(sql);
	    ArrayList<Inspection> retlist = new ArrayList();

        try {    
    	    while ( rs.next() ) {
                retlist.add( Inspection.createInspectionFromRS(rs) );
    	    }
        }
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}   

        return retlist;
    }

    public Algo getAlgoByAlgoNumber( final int algoNumber, final int level ) throws AnetException {
        
        final String sql = Algo.getAlgoSelectStmt()
            + " AND am.algo_number='" + algoNumber + "'"
            + " AND at.algo_level='" + level + "'"
            + " AND am.recipe_id='" + _recipeID + "'";
            
        ResultSet rs = Anet.get().sqlQuery(sql);

        try {
            if ( ! rs.next() ) {
                throw new AnetException( "Algonumber \"" + algoNumber + "\" was not found." );
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}

        return Algo.createAlgoFromRS( rs );
    }

    static public String getRecipeSelectStmt() {
     
        return "SELECT"
            + " recipe_name,"
            + " recipe_mod,"
            + " recipe_id";
    }
  
    static public Recipe createRecipeFromRS( ResultSet rs ) throws AnetException {	
    
        try {
        
            Recipe recipe = new Recipe();
     
            recipe.setName( rs.getString("recipe_name") );
            recipe.setModDate( rs.getLong("recipe_mod") );
            recipe.setRecipeID( rs.getInt("recipe_id") );  
                     
            return recipe;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}  
   }

 //   public AlgoChain getAlgoChain( final int level, final int algoNumber ) throws AnetException;

 //   public ArrayList<AlgoChain> getAlgos( final int level ) throws AnetException;

 //   public ArrayList<AlgoChain> getAlgos( final int level, final String algoType ) throws AnetException;

    public Group getGroup( final String refName ) throws AnetException {
    
        if ( refName == null || refName.trim().length() == 0 ) {
            throw new AnetException( "The group reference name was null or empty." );
        }
        
        String sql = Group.getGroupSelectStmt() 
            + " WHERE recipe_id='" + _recipeID + "'"
            + " AND gm.ref_name='" + refName + "'"
            + " LIMIT 0,1";

        ResultSet rs = Anet.get().sqlQuery(sql);

        try {
            if ( ! rs.next() ) {
                return null;
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}
        
        return Group.createGroupFromRS(rs);
    }

    public ArrayList<Group> getGroups() throws AnetException {
    
        String sql = Group.getGroupSelectStmt() 
            + " WHERE recipe_id='" + _recipeID + "'";

        ResultSet rs = Anet.get().sqlQuery(sql);   
        ArrayList<Group> retlist = new ArrayList();

        try {    
    	    while ( rs.next() ) {
                retlist.add( Group.createGroupFromRS(rs) );
    	    }
        }
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}   

        return retlist;
    }

    public ArrayList<Group> getGroups( final int algoNumber ) throws AnetException {
        return getGroups( getAlgoByAlgoNumber(algoNumber, Anet.LEVEL2) );
    }

    public ArrayList<Group> getGroups( final Algo algo ) throws AnetException {
    
        String sql = Group.getGroupSelectStmt() 
            + " WHERE recipe_id='" + _recipeID + "'"
            + " AND algoNumber='" + algo.getAlgoNumber() + "'";

        ResultSet rs = Anet.get().sqlQuery(sql);   
        ArrayList<Group> retlist = new ArrayList();

        try {    
    	    while ( rs.next() ) {
                retlist.add( Group.createGroupFromRS(rs) );
    	    }
        }
    	catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}   

        return retlist;
    }
 
    public String toString() {
        
        return "Recipe Name: " + _name + "\n"
            + "Mod Date: " + _modDate + "\n"
            + "Recipe ID: " + _recipeID;
    }

}
