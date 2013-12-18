package com.visionpro.anet;

public class ResultEntry {

    protected String _name = null;
    public String getName() { return _name; }
    public void setName( final String val ) { _name = val; }
    
    protected int _val = Anet.BAD_VAL;
    public int getVal() { return _val; }
    public void setVal( final int val ) { _val = val; }
    
    public String toString() {
        return Integer.toString( _val );
    }
}
