package com.visionpro.anet;

public class Item {

    protected int _pin = Anet.BAD_VAL;
    public int getPin() { return _pin; }
    public void setPin( final int val ) { _pin = val; }
    
    protected int _xLoc = Anet.BAD_VAL;
    public int getXLoc() { return _xLoc; }
    public void setXLoc( final int val ) { _xLoc = val; }

    protected int _yLoc = Anet.BAD_VAL;
    public int getYLoc() { return _yLoc; }
    public void setYLoc( final int val ) { _yLoc = val; }

    protected int _dxLoc = Anet.BAD_VAL;
    public int getDXLoc() { return _dxLoc; }
    public void setDXLoc( final int val ) { _dxLoc = val; }

    protected int _dyLoc = Anet.BAD_VAL;
    public int getDYLoc() { return _dyLoc; }
    public void setDYLoc( final int val ) { _dyLoc = val; }

    protected int _algoNumber = Anet.BAD_VAL;
    public int getAlgoNumber() { return _algoNumber; }
    public void setAlgoNumber( final int val ) { _algoNumber = val; }

    protected int _orientation = Anet.BAD_VAL;
    public int getOrientation() { return _orientation; }
    public void setOrientation( final int val ) { _orientation = val; }
    
    protected Boolean _isVisible = false;
    public Boolean isVisible() { return _isVisible; }
    public void isVisible( Boolean val ) { _isVisible = val; }
}
