package com.visionpro.anet;

import java.util.Map;

public class AnetChartParams
{
  protected Map< String, String[] > _map;
  
  public AnetChartParams( final Map map ) {
    _map = map;
  }
  
  public final String get( final String key ) {
    return _map.containsKey(key) && (_map.get(key)[0]).length() > 0 ? _map.get(key)[0] : null;
  }
}
  
