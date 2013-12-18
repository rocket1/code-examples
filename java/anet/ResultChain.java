package com.visionpro.anet;

import java.util.*;

public class ResultChain {

    protected ArrayList<Result> _resChain = new ArrayList();

    public Result getHead() throws AnetException {
        return getResult( 0 );
    }

    public Result getTail() throws AnetException {
        return getResult( _resChain.size() - 1 );
    }

    public Result getResult( final int ndx ) throws AnetException {

        if ( ndx < 0 || ndx > _resChain.size() - 1 ) {
            throw new AnetException( "Result chain index was invalid" );
        }

        return _resChain.get( ndx );
    }

    public void appendResult( final Result result ) throws AnetException {

        if ( result == null ) {
            throw new AnetException( "Result was null" );
        }     

        _resChain.add( result );
    }
    
    static public ResultChain dbStrToResultChain( final String chainResultStr ) throws AnetException  {

        ResultChain resultChain = new ResultChain();

        String[] algos = chainResultStr.split("&");

        if (algos.length == 0) {
            algos[0] = chainResultStr;
        }

        for (int i = 0; i < algos.length; i++) {

            String[] toks = algos[i].split(" ");
            
            Result result = new Result();
            
            for (int j = 0; j < toks.length; j++) {
                ResultEntry entry = new ResultEntry();
                entry.setName( "foo" );
                entry.setVal( Integer.parseInt(toks[j]) );
                result.appendEntry( entry );
            }

            resultChain.appendResult( result );
        }

        return resultChain;
    }
    
    public String toString() {
    
        String ret = new String();
        
        for ( Result result : _resChain ) {
            ret += result.toString() + "    ";
        }
        
        return ret;
    }
}
