package com.applications;

import com.sun.jdi.InternalException;

import javax.swing.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.stream.Stream;

public class FourDNF {
   public ArrayList<Clause> clauses = new ArrayList<>();
   public MFGSwingWorker caller;
   public int varsize = -1;
   public boolean Evaluate(boolean[] values)
   {
      for(Clause c : clauses)
      {
         boolean temp = true;
         for(Literal l : c.literals)
         {
            if (!temp) break;
            boolean lval = (l.varidx < values.length) && values[l.varidx];
            temp = l.neg != lval;
         }
         if (temp) return true;
      }
      return false;
   }
   public boolean Exists()
   {
      if(varsize == -1) Normalize();
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
      if(varsize == -1) Normalize();
      boolean[] tmpvar = new boolean[varsize];

      return tmpvar;
   }
   public long Count()
   {
      if(varsize == -1) Normalize();
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
      if(varsize == -1) Normalize();
      it.unimi.dsi.util.XorShift1024StarPhiRandom gen = new it.unimi.dsi.util.XorShift1024StarPhiRandom();
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
      return (double)hit/(double)count;
   }
   //Importance Sampling -- Dealing with the low probability cases
   public double ApproximateRatioIS()
   {
      if(varsize == -1) Normalize();
      it.unimi.dsi.util.XorShift1024StarPhiRandom gen = new it.unimi.dsi.util.XorShift1024StarPhiRandom();
      boolean[] tmpvar = new boolean[varsize];
      long hit = 0;
      long count = 0;
      long total = 100000;
      // compute |U'|
      long szup = 0;
      for(Clause c: clauses)
      {
         szup += Math.pow(2,varsize-c.literals.size());
      }
      if(clauses.size() <= 0) return 1.0;
      while(count <total)
      {
         // pick a clause
         int cind = gen.nextInt(clauses.size());
         HashSet<Integer> lset = new HashSet<>();
         for(Literal l: clauses.get(cind).literals)
         {
            lset.add(l.varidx);
            tmpvar[l.varidx] = !l.neg;
         }
         for(int i = 0;i<varsize;++i)
         {
            if(lset.contains(i)) continue;;
            tmpvar[i] = gen.nextBoolean();
         }
         boolean inch = true;
         for(int i = 0;i<cind;++i)
         {
            boolean temp = true;
            for(Literal l : clauses.get(i).literals)
            {
               if (!temp) break;
               boolean lval = (l.varidx < varsize) ? tmpvar[l.varidx] : false;
               temp = l.neg != lval;
            }
            if (temp)
               inch = false;
         }
         if(inch) hit++;
         count++;
      }
      double factor = (double)szup/Math.pow(2,varsize);
      return (double)hit/(double)count * factor;
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

   //This will normalize the indices so that the dnf will not have index holes.
   public void Normalize()
   {
      int vars = 0;
      for (Clause c : clauses)
         for (Literal l : c.literals)
         {
            vars = Math.max(vars, l.varidx + 1);
         }

      boolean[] vs = new boolean[vars];
      for(Clause c: clauses)
         for(Literal l: c.literals)
         {
            vs[l.varidx] = true;
         }
      HashMap<Integer,Integer> nmap = new HashMap<>();
      int space = 0;
      for(int i = 0;i<vars;++i)
      {
         if(!vs[i])
         {
            ++space;
         }
         else
         {
            nmap.put(i,i-space);
         }
      }
      varsize = vars - space;
      for(Clause c: clauses)
         for(Literal l:c.literals)
         {
            l.varidx = nmap.get(l.varidx);
         }
   }
}
