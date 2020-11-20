package com.applications;

import java.util.ArrayList;

public class CNF {
   public ArrayList<Clause> clauses = new ArrayList<>();
   public boolean Evaluate(boolean[] v){
      if(clauses.size()<=0) return false;
      for(Clause c: clauses)
      {
         if(c.literals.size()<=0) continue;
         boolean clauseres = false;
         for(Integer i: c.literals)
         {
            int idx = Math.abs(i) - 1;
            boolean neg = i < 0;
            clauseres = neg != v[idx];
            if(clauseres) break;
         }
         if(!clauseres) return false;
      }
      return true;
   }
   public CNF ResolveFull(){
      CNF ret = new CNF();
      ret.clauses.addAll(clauses);

      return ret;
   }
}
