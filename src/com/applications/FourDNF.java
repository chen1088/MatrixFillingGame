package com.applications;

import com.sun.mail.iap.Literal;

import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class FourDNF {
   public ArrayList<Clause> clauses = new ArrayList<>();
   public MFGSwingWorker caller;
   public int varsize = -1;
   public boolean Evaluate(boolean[] values)
   {
      for(Clause c : clauses)
      {
         boolean temp = true;
         for(Integer l : c.literals)
         {
            if (!temp) break;
            int varidx = Math.abs(l) - 1;
            boolean neg = l < 0;
            boolean lval = (varidx < values.length) && values[varidx];
            temp = neg != lval;
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
      if(varsize == 0) return 0.0;
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
         for(Integer l: clauses.get(cind).literals)
         {
            int varidx = Math.abs(l) - 1;
            boolean neg = l < 0;
            lset.add(varidx);
            tmpvar[varidx] = !neg;
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
            for(Integer l : clauses.get(i).literals)
            {
               if (!temp) break;
               int varidx = Math.abs(l) - 1;
               boolean neg = l < 0;
               boolean lval = (varidx < varsize) ? tmpvar[varidx] : false;
               temp = neg != lval;
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
         for (Integer l : c.literals)
         {
            int varidx = Math.abs(l) - 1;
            vars = Math.max(vars, varidx + 1);
         }

      boolean[] vs = new boolean[vars];
      for(Clause c: clauses)
         for(Integer l: c.literals)
         {
            int varidx = Math.abs(l) - 1;
            vs[varidx] = true;
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
         for(Integer l:c.literals)
         {
            int varidx = Math.abs(l) - 1;
            varidx = nmap.get(varidx);
         }
   }
   public void ToTextPaneAsCNFStr(JTextPane pane) {
      // Create the StyleContext, the document and the pane
      StyleContext sc = new StyleContext();
      TabStop[] tabs = new TabStop[5];
      tabs[0] = new TabStop(100, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
      tabs[1] = new TabStop(200, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
      tabs[2] = new TabStop(300, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
      tabs[3] = new TabStop(400, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
      tabs[4] = new TabStop(500, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
      TabSet tabset = new TabSet(tabs);
      AttributeSet aset = sc.addAttribute(SimpleAttributeSet.EMPTY, StyleConstants.TabSet, tabset);
      final DefaultStyledDocument doc = new DefaultStyledDocument(sc);
      doc.setParagraphAttributes(0,0,aset,false);
      pane.setDocument(doc);
      // Create and add the style

      final Style pstyle = sc.addStyle("pos",null);
      pstyle.addAttribute(StyleConstants.FontSize, 16);
      pstyle.addAttribute(StyleConstants.Foreground, Color.red);

      final Style nstyle = sc.addStyle("neg", null);
      nstyle.addAttribute(StyleConstants.FontSize, 16);
      nstyle.addAttribute(StyleConstants.Foreground, Color.blue);
      nstyle.addAttribute(StyleConstants.Underline, true);

      for(int i = 0;i<clauses.size();++i)
      {
         Clause c = clauses.get(i);
         ArrayList<Integer> ls = c.literals;
         for(int j = 0;j<ls.size();++j)
         {
            String nstr = Integer.toString(Math.abs(ls.get(j)));
            Style sty = ls.get(j) < 0?pstyle:nstyle;

            try{
               doc.insertString(doc.getLength(),nstr,sty);
               doc.insertString(doc.getLength()," ",null);
            }catch (BadLocationException e)
            {
               System.out.print("!\n");
            }
         }
         try {
            doc.insertString(doc.getLength(),i%5!=4?"\t":"\n",null);

         }catch (BadLocationException e)
         {

         }
      }
   }
}
