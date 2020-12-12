package framework.combinatorics;

public class Util {
   static int PairingFunction(int x, int y)
   {
      int z = x+y;
      return z*(z+1)/2 + y;
   }
}
