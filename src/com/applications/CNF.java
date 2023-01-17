package com.applications;

import framework.combinatorics.DFA;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

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
      //TODO:
      CNF ret = new CNF();
      ret.clauses.addAll(clauses);

      return ret;
   }
   public static DFA convertCNFClausetoDFA(Clause clause)
   {
      //TODO:
      DFA ret = new DFA();
      Set<Integer> literals = new HashSet<>(clause.literals);
      //ArrayList<Integer> literals = clause.literals;

      return ret;
   }
}
