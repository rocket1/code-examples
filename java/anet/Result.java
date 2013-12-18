package com.visionpro.anet;

import java.util.*;

public class Result {

    protected ArrayList<ResultEntry> _resEntries = new ArrayList();

    public ResultEntry getEntry( final int ndx ) throws AnetException {

        if ( ndx < 0 || ndx > _resEntries.size() - 1 ) {
           // throw new AnetException( "Result entry index was invalid" );
           ResultEntry badEntry = new ResultEntry();
           badEntry.setVal(0);
           return badEntry;
        }

        return _resEntries.get( ndx );
    }

    public void appendEntry( final ResultEntry entry ) throws AnetException {

        if ( entry == null ) {
            throw new AnetException( "Result entry was null" );
        }     

        _resEntries.add( entry );
    }
    
    public ArrayList<ResultEntry> getResultEntries() {
        return _resEntries;
    }
   
    public String toString() {
        
        String ret = new String();
        
        for ( ResultEntry resEntry : _resEntries ) {
            ret += resEntry.toString() + " ";
        }
        
        return ret;
    }
}    
