package com.visionpro.anet;

import java.util.*;
import java.text.*;

public class AnetUtil {

    static public long HOURS_1  = 60 * 60;
    static public long HOURS_24 = HOURS_1 * 24;

    static public ArrayList<Integer> str2IntArray( final String str ) throws AnetException {
    
        ArrayList<Integer> ret = new ArrayList();
        
        if ( str != null && str.trim().length() > 0 ) {
        
            String[] toks = str.trim().split(" ");
    
            for ( int i = 0; i < toks.length; i++ ) {
                ret.add( Integer.parseInt( toks[i] ) );
            }
        }

        return ret;
    }
    
    static public String timestamp2Str( final long timestamp ) {
    
        java.util.Date date = new java.util.Date( timestamp * 1000 );
        Format formatter = new SimpleDateFormat( "E, dd MMM yyyy HH:mm:ss" );
        return formatter.format(date);     
    }  
}
