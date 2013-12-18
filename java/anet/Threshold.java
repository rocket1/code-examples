package com.visionpro.anet;

import java.util.*;

class Threshold {

    protected ArrayList<ThresholdEntry> th_entries = new ArrayList();

    public ThresholdEntry getEntry( final int ndx ) throws AnetException {
    
        if ( ndx < 0 || ndx > th_entries.size() - 1 ) {
            throw new AnetException( "Threshold entry index (" + ndx + ") was invalid (" + Integer.toString((th_entries.size() - 1)) + " max)" );
        }
    
        return th_entries.get( ndx );
    }
    
    public void appendEntry( final ThresholdEntry entry ) throws AnetException {
    
        if ( entry == null ) {
            throw new AnetException( "Threshold entry was null" );
        }     
    
        th_entries.add( entry );
    }
    
    static public Threshold dbStrToThreshold( final String th_str ) throws AnetException {
        
        if ( th_str == null ) {
            throw new AnetException( "Threshold database string was null." );
        }
        
        String[] th_vals = th_str.split(" ");
        Threshold th = new Threshold();
        
        for (int i = 0; i < th_vals.length; ++i) {
            ThresholdEntry th_entry = new ThresholdEntry();
            th_entry.setName( "foo" );
            th_entry.setVal( Integer.parseInt(th_vals[i]) );
            th.appendEntry( th_entry );       
        }
        
        return th;
    }
}
