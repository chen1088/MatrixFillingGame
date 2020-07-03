package com.applications;

import com.sun.jdi.InternalException;

import javax.swing.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.stream.Stream;

public class FourDNF {
   public ArrayList<Clause> clauses = new ArrayList<>();
   public MFGSwingWorker caller;
   public int varsize;
   public int numofclauses;
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
   public boolean[] Greedy()
   {
      //TODO: Polynomial algorithm
      int varsize = 0;
      for(Clause c : clauses)
         for(Literal l : c.literals)
         {
            varsize = Math.max(varsize, l.varidx + 1);
         }
      boolean[] tmpvar = new boolean[varsize];

      return tmpvar;
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
      final long checkintinterval = 10000;
      long counter = 0;
      long current = 0;
      while (!EqualsOne(tmpvar))
      {
         if(counter >= checkintinterval) {
            counter = 0;
            if(caller.isCancelled()) return -1;
            if(caller!=null) caller.Publish((int)Math.floor(100.0*(double)current/Math.pow(2,varsize)));
         }
         boolean tmp = Evaluate(tmpvar);
         if (tmp) ++ret;
         IncByOne(tmpvar);
         current++;
         counter++;
      }
      if (Evaluate(tmpvar)) ++ret;// for all one case
      return ret;
   }
   // Basic approximation
   public double ApproximateRatio()
   {
      it.unimi.dsi.util.XorShift1024StarPhiRandom gen = new it.unimi.dsi.util.XorShift1024StarPhiRandom();
      double res = 0.0;
      int varsize = 0;
      for (Clause c : clauses)
         for (Literal l : c.literals)
         {
            varsize = Math.max(varsize, l.varidx + 1);
         }
      boolean[] tmpvar = new boolean[varsize];
      long hit = 0;
      long count = 0;
      long total = 100000;
      while(count < total)
      {
         // randomize an assignment
         for(int i = 0;i<varsize;++i)
         {
            tmpvar[i] = gen.nextBoolean();
         }
         if(Evaluate(tmpvar)) hit++;
         count++;
      }
      return (double)hit/(double)total;
   }
   //Importance Sampling -- Dealing with the low probability cases
   public double ApproximateRatioIS()
   {
      it.unimi.dsi.util.XorShift1024StarPhiRandom gen = new it.unimi.dsi.util.XorShift1024StarPhiRandom();
      
      return 0.0;
   }
   //Smart approximation -- Smarter than importance sampling
   public double SmartApproximationRatio()
   {

      return 0.0;
   }

   public static void IncByOne(boolean[] vs)
   {
      for(int i = 0;i<vs.length;++i)
      {
         vs[i] = !vs[i];
         if (vs[i]) return;
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
