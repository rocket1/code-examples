package com.visionpro.anet;

import java.util.*;
import java.sql.*;
import java.io.*;
import java.lang.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.http.*;

import com.jolbox.bonecp.*;

public class AnetConnection {

    BoneCP _pool = null;
    private Connection _con = null;
    static public ServletContext _sc = null;
    
    private long _queryTime = 0;
    public long getQueryTime() { return _queryTime; }
    
    private String _driver = null;
    public String getDriver() { return _driver; }
    public void setDriver( final String val ) { _driver = val; }

    private String _host = null;
    public String getHost() { return _host; }
    public void setHost( final String val ) { _host = val; }

    private String _user = null;
    public String getUser() { return _user; }
    public void setUser( final String val ) { _user = val; }

    private String _db = null;
    public String getDB() { return _db; }
    public void setDB( final String val ) { _db = val; }

    private String _pass = null;
    public String getPass() { return _pass; }
    public void setPass( final String val ) { _pass = val; }
    
    private Boolean _debugSql = true;
    public Boolean getDebugSql() { return _debugSql; }
    public void setDebugSql( final Boolean val ) { _debugSql = val; }
    
    private Boolean _usePool = false;
    public void usePool( final Boolean val ) { _usePool = val; }

    public void connect() throws AnetException {
        throw new AnetException( "Connect with no parameters not supported." );
    }

    static private void log( final String msg ) {
        if ( _sc != null ) {
            _sc.log(msg);
        }
    }

    public void connect( String driver,
                         String host,
                         String user,
                         String pass,
                         String db ) throws AnetException {

        if ( System.getenv( "ANET_DRIVER" ) != null ) {

            driver = System.getenv( "ANET_DRIVER" );
            host   = System.getenv( "ANET_HOST" );
            user   = System.getenv( "ANET_USER" );
            pass   = System.getenv( "ANET_PASS" );
            db     = System.getenv( "ANET_DB" );
        }

        // Sanitize and store login credentials.
        _driver = (driver == null || driver.trim().equals("")) ? null : driver.trim();
        _host   = (host   == null || host  .trim().equals("")) ? null : host  .trim();
        _user   = (user   == null || user  .trim().equals("")) ? null : user  .trim();
        _db     = (db     == null || db    .trim().equals("")) ? null : db    .trim();
        _pass   = (pass   == null || pass  .trim().equals("")) ? null : pass  .trim();

        String driver_class = null;
        String dsn = null;

        try {

            close();

            if ( _driver == null ) {
                throw new AnetException( "Database driver was not set." );
            }

            if ( _driver.equals("mysql") ) {
                
                if ( _host == null ) {
                    throw new AnetException( "MySQL hostname was empty." );
                }

                if ( _user == null ) {
                    throw new AnetException( "MySQL user was empty." );
                }
                
                if ( _pass == null ) {
                    throw new AnetException( "MySQL password was empty." );
                }    
             
                if ( _db == null ) {
                    throw new AnetException( "MySQL database name was not set." );
                }
                
                driver_class = "com.mysql.jdbc.Driver";
                Class.forName( driver_class );
                dsn = "jdbc:mysql://" + host + ":3306/" + _db;

		if ( _usePool ) {

		    if ( _pool == null ) {

			BoneCPConfig config = new BoneCPConfig();

			config.setJdbcUrl( dsn );
			config.setUsername( user ); 
			config.setPassword( pass );
			config.setMinConnectionsPerPartition( 5 );
			config.setMaxConnectionsPerPartition( 10 );
			config.setPartitionCount( 1 );
			
			_pool = new BoneCP( config );
			Anet.p( "Using *NEW* BoneCPConfig." );
		    }
		    else {
			Anet.p( "Using existing BoneCPConfig." );
		    }
			
		    _con = _pool.getConnection();
		}
		else { 
		    _con = DriverManager.getConnection( dsn, user, pass );
		}
            }
            else if ( driver.equals("sqlite") ) {
        	
                if ( _db == null ) {
                    throw new AnetException( "SQLite database name was not set." );
                }

                // Does the .sqlite file exist on disk?
                File f = new File( _db );
                
                if( ! f.exists() ) {
                    throw new AnetException( "SQLite database \"" + _db + "\" was not found." );    
                }
		
		driver_class = "org.sqlite.JDBC";
		Class.forName( driver_class );
		dsn = "jdbc:sqlite:" + _db;
		_con = DriverManager.getConnection( dsn );
            }
            
            if ( _con == null ) {
                throw new AnetException( "Failed to connect." );
            }
        }
        catch ( ClassNotFoundException e ) {
            throw new AnetException(e);
        }
        catch ( SQLException e ) {
            throw new AnetException(e);
        }
    }
    
    public void close() throws AnetException {

    	try {
    	
    	   if ( _con != null ) {
    	       _con.close();
    	   }
    	   
    	   _con = null;
    	}
    	catch ( SQLException e) {
    	    throw new AnetException(e);
    	}
    }
    
    public ResultSet sqlQuery( String sql )	throws AnetException {

        try {

	    long then = System.currentTimeMillis();
	    Statement stmt = _con.createStatement();
	    ResultSet rs = stmt.executeQuery(sql);
	    long now = System.currentTimeMillis();
            _queryTime = now - then;
	    float qryTime = (float)_queryTime * .001F;

	    if ( qryTime > 3 ) {
		log( sql + " LONG " + Float.toString(qryTime) + " sec" );
	    }

	    return rs;
        }
        catch ( SQLException e ) {
    	    throw new AnetException(e);
    	}        
    }
    
    protected void finalize() throws Throwable {
	
    	try {
    	    close();
    	}
    	catch ( Exception e ) {
    	    throw e;
    	}
    	finally {
    	    super.finalize();
    	}
    }

}
