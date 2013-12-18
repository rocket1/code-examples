package com.visionpro.anet;

import java.util.*;
import java.sql.*;
import java.io.*;
import java.lang.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.http.*;

/** Description of Anet 
 *
 * @author John Doe
 * @author Jane Doe
 * @version 6.0z Build 9000 Jan 3, 1970.
 */
public class Anet {

    public final static int FRONT_LANE = 0;
    public final static int BACK_LANE = 1;

    public final static int LEVEL1 = 1;
    public final static int LEVEL2 = 2;

    public final static int BAD_VAL = Integer.MIN_VALUE;

    static public ServletContext _servletContext = null;
    static private AnetConnection _anetConn = new AnetConnection();
    
    static private Anet _instance;

    public Anet() {
        _anetConn.setDebugSql( System.getenv( "ANET_DEBUG" ) != null );
    }

    static public Anet get() {
    
        if ( _instance == null ) {
            _instance = new Anet();
        }
        
        return _instance;
    }
    
    static public Anet getAndConnect() throws AnetException {
    
        if ( _instance == null ) {
            _instance = new Anet();
            _anetConn.connect();
        }
        
        return _instance;
    }

    public void setConnectionModel( final AnetConnection model ) {
        _anetConn = model;
    }   

    public AnetConnection getConnectionModel() {
        return _anetConn;
    }  

    public Recipe getRecipeByID( final int recipeID ) throws AnetException {
    
        final String sql = "SELECT"
            + " *"
            + " FROM recipe_meta"
            + " WHERE recipe_id='" + recipeID + "'";
            
        ResultSet rs = sqlQuery(sql);
        
        try {
            if ( ! rs.next() ) {
                throw new AnetException( "The recipe by recipe_id was not available." );
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}
    	
        return Recipe.createRecipeFromRS(rs);
    }

    public Recipe getLastRecipe() throws AnetException {
        return getLastInspection().getRecipe();
    }
    
    public ArrayList<Recipe> getRecipes() throws AnetException {
        return getRecipes( localhost() );
    }
    
    public ArrayList<Recipe> getRecipes( final String tool ) throws AnetException { 

        final String subSql = "SELECT DISTINCT"
            + " recipe_id"
            + " FROM insp_hdr"
            + " WHERE machine='" + tool + "'";

    	final String sql = Recipe.getRecipeSelectStmt()
    	   + " FROM recipe_meta"
           + " WHERE recipe_id IN( " + subSql + " )"
           + " ORDER BY recipe_mod"
           + " DESC";

	    ResultSet rs = sqlQuery(sql);
	    ArrayList<Recipe> retlist = new ArrayList();
	    
	    try {
    	    while ( rs.next() ) {
        		retlist.add( Recipe.createRecipeFromRS(rs) );
    	    }
    	}
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	} 
	    
	    return retlist;
    }

    public Inspection getLastInspection() throws AnetException {
        return getLastInspection( localhost() );
    }

    public Inspection getLastInspection( final String tool ) throws AnetException {
    
        final String sql = Inspection.getInspectionSelectStmt()
            + " AND ihdr.machine='" + tool + "'"
            + " ORDER BY insp_id"
            + " DESC"
            + " LIMIT 0,1";
        
        ResultSet rs = sqlQuery(sql);
	    
        try {
            if ( ! rs.next() ) {
                throw new AnetException( "The last inspection was not available." );
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}

        return Inspection.createInspectionFromRS( rs );
    }

    public ArrayList<Inspection> getInspections( final int limit ) throws AnetException {
        return getInspections( localhost(), getLastRecipe(), limit );
    }
    
    public ArrayList<Inspection> getInspections( final String tool,
                                                 final Recipe recipe,
                                                 final int limit ) throws AnetException {
        
        ArrayList<Inspection> insps = new ArrayList();
        
        final String sql = Inspection.getInspectionSelectStmt()
            + " AND ihdr.machine='" + tool + "'"
            + " AND ihdr.recipe_id='" + Integer.toString(recipe.getRecipeID()) + "'"
            + " ORDER BY insp_id"
            + " DESC"
            + " LIMIT 0," + Integer.toString(limit);
            
        ResultSet rs = sqlQuery(sql);
	    
        try {
        
            while ( rs.next() ) {
                insps.add( Inspection.createInspectionFromRS(rs) );
            }
            
            if ( insps.size() < 1 ) {
                throw new AnetException( "The inspections were not available." );
            }
        
            return insps;    
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}    
    }   

    public Inspection getInspectionByBarcode( final String barcode ) throws AnetException {

        final String sql = Inspection.getInspectionSelectStmt()
            + " AND ihdr.serial='" + barcode + "'"
            + " LIMIT 0,1";

        ResultSet rs = sqlQuery(sql);
        
        try {	    
            if ( ! rs.next() ) {
                return null;
            }
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}

        return Inspection.createInspectionFromRS( rs );
    }

//    public Panel getPanelByBarcode( final String barcode ) throws AnetException {}

    
    public void setServletContext( ServletContext sc ) {
        _servletContext = sc;
    }
    
    public void log( final String msg ) {

    	if ( _servletContext != null ) {
    	    _servletContext.log(msg);
    	}

    	System.err.println(msg);
    }
    
    static public void p( final Object o ) {
        System.out.println( o == null ? "MSG WAS NULL." : o.toString() );
    }
    
    public long queryTime() {
        return _anetConn.getQueryTime();
    }
    
    static public String localhost() throws AnetException {
        
        try {
            return InetAddress.getLocalHost().getHostName();
        }
        catch (UnknownHostException e) {
            throw new AnetException(e);
        }
    }
    
    public void dumpCredentials() {
    
        p( "Driver: " + _anetConn.getDriver() );
        p( "Host: "   + _anetConn.getHost()   );
        p( "User: "   + _anetConn.getUser()   );
        p( "Pass: "   + _anetConn.getPass()   );
        p( "DB: "     + _anetConn.getDB()     );
    }
    
    public ResultSet sqlQuery( String sql )	throws AnetException {
        return _anetConn.sqlQuery( sql );   
    }
   
    public void connect() throws AnetException {
        _anetConn.connect();
   }
   
   public void close() throws AnetException {
        _anetConn.close();
   }
   
    public static void main( String[] args ) {
        try {
            Anet anet = Anet.getAndConnect();           
            ArrayList<Recipe> recipes = anet.getRecipes();
            
            for ( Recipe recipe : recipes ) {
                p( recipe.toString() );
            }
            
            Inspection insp = anet.getLastInspection();
            p( insp.toString() );
            p( insp.getRecipe().toString() );
            
            ArrayList<Integer> values = insp.getResultByAlgoNumber( 4, Anet.LEVEL1, 9 );
            
            for ( Integer i : values ) {
                if ( i == 0 ) {
                    continue;
                }
              //  p( i );
            }
            
            ArrayList<InspItem> inspItems = insp.getInspItemsByAlgoName( "volume" );
            
            for ( InspItem i : inspItems ) {
                p( i.toString() );
            }
            
            ArrayList<Group> groups = anet.getLastRecipe().getGroups();
            
            for ( Group group : groups ) {
                p( group.toString() );
            }
        }
        catch (AnetException e) {
            e.printStackTrace();
        }
    }
}
