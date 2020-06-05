package com.applications;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.stream.Stream;

public class FourDNF {
   public ArrayList<Clause> clauses = new ArrayList<>();
   public boolean Evaluate(boolean[] values)
   {
      for(Clause c : clauses)
      {
         boolean temp = true;
         for(Literal l : c.literals)
         {
            if (!temp) break;
            boolean lval = (l.varidx < values.length) ? values[l.varidx] : false;
            temp = l.neg != lval;
         }
         if (temp) return true;
      }
      return false;
   }
   public boolean Exists()
   {
      int varsize = 0;
      for(Clause c : clauses)
      for(Literal l : c.literals)
      {
         varsize = Math.max(varsize, l.varidx + 1);
      }
      boolean[] tmpvar = new boolean[varsize];
      while(!EqualsOne(tmpvar))
      {
         boolean tmp = Evaluate(tmpvar);
         if (tmp) return true;
         IncByOne(tmpvar);
      }
      return Evaluate(tmpvar);// for all one case
   }
   public long Count()
   {
      int varsize = 0;
      for (Clause c : clauses)
      for (Literal l : c.literals)
      {
         varsize = Math.max(varsize, l.varidx + 1);
      }
      boolean[] tmpvar = new boolean[varsize];
      long ret = 0;
      while (!EqualsOne(tmpvar))
      {
         boolean tmp = Evaluate(tmpvar);
         if (tmp) ++ret;
         IncByOne(tmpvar);
      }
      if (Evaluate(tmpvar)) ++ret;// for all one case
      return ret;
   }
   public double ApproximateRatio()
   {
      return 0.0;
   }

   public static void IncByOne(boolean[] vs)
   {
      boolean carry = true;
      for(int i = 0;i<vs.length;++i)
      {
         boolean nv = !vs[i];
         carry = vs[i];
         vs[i]=nv;
         if (!carry) return;
      }
   }

   public static boolean EqualsOne(boolean[] vs)
   {
      for(int i = 0;i<vs.length;++i) {
         if(!vs[i])
            return false;
      }
      return true;
   }
}
