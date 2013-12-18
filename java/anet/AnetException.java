package com.visionpro.anet;

public class AnetException extends Exception
{
    public AnetException( final String msg ) {
        super( msg );
    }

    public AnetException( Exception e ) {
        super( e.getMessage() );
    }
}
