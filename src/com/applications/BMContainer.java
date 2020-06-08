package com.applications;

import javafx.util.Pair;

import java.util.ArrayList;
import java.util.Stack;

public class BMContainer {
   private ArrayList<BMEntry> matrix;
   //private ArrayList<Command> commands = new ArrayList<Command>();
   private ArrayList<Integer> ignorerows;
   private ArrayList<Integer> ignorecols;
   private final ArrayList<Pair<Integer, Integer>> ignoreentries;
   private Integer height;
   private Integer width;
   private final Stack<ArrayList<BMEntry>> matrixhistory;
   private final Stack<ArrayList<Integer>> ignorerowshistory;
   private final Stack<ArrayList<Integer>> ignorecolshistory;
   private final Stack<ArrayList<Pair<Integer, Integer>>> ignoreentrieshistory;
   private final Stack<Integer> heighthistory;
   private final Stack<Integer> widthhistory;

   public BMContainer() {
      matrix = new ArrayList<>();
      ignorerows = new ArrayList<>();
      ignorecols = new ArrayList<>();
      ignoreentries = new ArrayList<>();
      matrixhistory = new Stack<>();
      ignorerowshistory = new Stack<>();
      ignorecolshistory = new Stack<>();
      ignoreentrieshistory = new Stack<>();
      heighthistory = new Stack<>();
      widthhistory = new Stack<>();
      height = 6;
      width = 6;
   }

   // format (height);(width);(data:r,c,v/);(mask:r,c/);(row#/);(col#/)
   public void Clear()
   {
      matrix.clear();
      ignorerows.clear();
      ignorecols.clear();
      ignoreentries.clear();
      height = 6;
      width = 6;
      matrixhistory.clear();
      ignorerowshistory.clear();
      ignorecolshistory.clear();
      ignoreentrieshistory.clear();
      heighthistory.clear();
      widthhistory.clear();
   }
   public void FromString(String input)
   {
      Clear();
      String[] parts = input.split(";");
      Integer height = Integer.parseInt(parts[0]);
      Integer width = Integer.parseInt(parts[1]);
      String[] data = parts[2].split("/");
      for(String d : data)
      {
         if ("".equals(d)) continue;
         String[] e = d.split("'");
         Integer r = Integer.parseInt(e[0]);
         Integer c = Integer.parseInt(e[1]);
         int v = Integer.parseInt(e[2]);
         matrix.add(new BMEntry(r, c, v));
      }
      String[] mask = parts[3].split("/");
      for(String m : mask)
      {
         if ("".equals(m)) continue;
         String[] i = m.split(",");
         Integer r = Integer.parseInt(i[0]);
         Integer c = Integer.parseInt(i[1]);
         ignoreentries.add(new Pair<>(r, c));
      }
      String[] rs = parts[4].split("/");
      for(String r : rs)
      {
         if ("".equals(r)) continue;
         Integer rn = Integer.parseInt(r);
         ignorerows.add(rn);
      }
      String[] cs = parts[5].split("/");
      for(String c : cs)
      {
         if ("".equals(c)) continue;
         Integer cn = Integer.parseInt(c);
         ignorecols.add(cn);
      }
   }
   public String ToString()
   {
      StringBuilder ret = new StringBuilder();
      for (BMEntry d : matrix)
      {
         ret.append(d.rIdx).append(',').append(d.cIdx).append(',').append(d.val).append('/');
      }
      ret.append(';');
      for (Pair<Integer,Integer> m : ignoreentries)
      {
         ret.append(m.getKey()).append(',').append(m.getValue()).append('/');
      }
      ret.append(';');
      for (Integer r : ignorerows)
      {
         ret.append(r).append('/');
      }
      ret.append(';');
      for (Integer c : ignorecols)
      {
         ret.append(c).append('/');
      }
      return ret.toString();
   }
   public void SetEntry(Integer r, Integer c, Integer val)
   {
      saveSnapshot();
      // find if the entry exists first
      for(BMEntry e : matrix)
      {
         if(e.cIdx == c && e.rIdx == r)
         {
            e.val = val;
            return ;
         }
      }
      BMEntry n = new BMEntry(r, c, val);
      matrix.add(n);
   }
   public void SetIgnore(Integer r, Integer c, boolean ignore)
   {
      for(Pair<Integer,Integer> t : ignoreentries)
      {
         if(t.getKey().equals(r) && t.getValue().equals(c))
         {
            if (!ignore)
            {
               ignoreentries.remove(t);
            }
            return;
         }
      }
      if(ignore)
         ignoreentries.add(new Pair<>(r, c));
   }
   public void SetIgnorerow(Integer row, boolean ignore)
   {
      for(Integer i : ignorerows)
      {
         if(i.equals(row) && !ignore)
         {
            ignorerows.remove(i);
            return;
         }
      }
      if (ignore)
         ignorerows.add(row);
   }
   public void SetIgnorecol(Integer col, boolean ignore)
   {
      for (Integer i : ignorecols)
      {
         if (i.equals(col) && !ignore)
         {
            ignorecols.remove(i);
            return;
         }
      }
      if (ignore)
         ignorecols.add(col);
   }
   public Integer[][] GetFullMatrix()
   {
      Integer[][] ret = new Integer[height][width];
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            ret[i][j] = 2;
      for(BMEntry e : matrix)
      {
         ret[e.rIdx][e.cIdx] = e.val;
      }
      return ret;
   }
   public boolean[][] GetFullIgnoreMap()
   {
      boolean[][] ret = new boolean[height][width];
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            ret[i][j] = false;
      for (Integer i : ignorerows)
      for (int j = 0; j < width; ++j)
         ret[i][j] = true;
      for (Integer i : ignorecols)
      for (int j = 0; j < height; ++j)
         ret[j][i] = true;
      for (Pair<Integer, Integer> t : ignoreentries)
      ret[t.getKey()][t.getValue()] = true;
      return ret;
   }
   public boolean[][] GetForcedMap()
   {
      boolean[][] ret = new boolean[height][width];
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            ret[i][j] = false;
      Integer[][] m = GetFullMatrix();
      boolean[][] h1map = new boolean[width][width];//10
      boolean[][] h2map = new boolean[width][width];//01
      for(int i = 0; i<height; ++i)
         for(int j = 0; j<width; ++j)
            for(int k = j+1; k<width; ++k)
            {
               if(m[i][j] == 1 && m[i][k] == 0)
               {
                  h1map[j][k] = true;
               }
                        else if(m[i][j] == 2 && m[i][k] == 1 && h1map[j][k])
               {
                  ret[i][j] = true;
               }
                        else if(m[i][j] == 0 && m[i][k] == 2 && h1map[j][k])
               {
                  ret[i][k] = true;
               }
            }
      for(int i = height-1; i>=0; --i)
         for(int j = 0; j<width; ++j)
            for(int k = j+1; k<width; ++k)
            {
               if (m[i][j] == 0 && m[i][k] == 1)
               {
                  h2map[j][k] = true;
               }
                        else if (m[i][j] == 2 && m[i][k] == 0 && h2map[j][k])
               {
                  ret[i][j] = true;
               }
                        else if (m[i][j] == 1 && m[i][k] == 2 && h2map[j][k])
               {
                  ret[i][k] = true;
               }
            }
      return ret;
   }
   public boolean[][] GetConflictMap()
   {
      boolean[][] ret = new boolean[height][width];
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            ret[i][j] = false;
      boolean[][] oneimp = new boolean[height][width];
      boolean[][] zeroimp = new boolean[height][width];
      Integer[][] m = GetFullMatrix();
      boolean[][] h1map = new boolean[width][width];//10
      boolean[][] h2map = new boolean[width][width];//01
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            for (int k = j + 1; k < width; ++k)
            {
               if (m[i][j] == 1 && m[i][k] == 0)
               {
                  h1map[j][k] = true;
               }
                        else if (m[i][j] == 2 && m[i][k] == 1 && h1map[j][k])
               {
                  zeroimp[i][j] = true;
               }
                        else if (m[i][j] == 0 && m[i][k] == 2 && h1map[j][k])
               {
                  oneimp[i][k] = true;
               }
            }
      for (int i = height - 1; i >= 0; --i)
         for (int j = 0; j < width; ++j)
            for (int k = j + 1; k < width; ++k)
            {
               if (m[i][j] == 0 && m[i][k] == 1)
               {
                  h2map[j][k] = true;
               }
                        else if (m[i][j] == 2 && m[i][k] == 0 && h2map[j][k])
               {
                  oneimp[i][j] = true;
               }
                        else if (m[i][j] == 1 && m[i][k] == 2 && h2map[j][k])
               {
                  zeroimp[i][k] = true;
               }
            }
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            ret[i][j] = zeroimp[i][j] & oneimp[i][j];
      return ret;
   }

   public boolean HasDiagRect()
   {

      return false;
   }
   public void InsertRow(Integer idx)
   {
      saveSnapshot();
      ++height;
      for(BMEntry e : matrix)
      {
         if (e.rIdx >= idx)
            ++e.rIdx;
      }
   }
   public void InsertCol(Integer idx)
   {
      saveSnapshot();
      ++width;
      for (BMEntry e : matrix)
      {
         if (e.cIdx >= idx)
            ++e.cIdx;
      }
   }
   public void RemoveRow(Integer idx)
   {
      saveSnapshot();
      --height;
      ArrayList<BMEntry> removeArrayList = new ArrayList<>();
      for(BMEntry e : matrix)
      {
         if(e.rIdx == idx)
         {
            removeArrayList.add(e);
         }
         else if(e.rIdx > idx)
         {
            --e.rIdx;
         }
      }
      for (BMEntry e : removeArrayList)
         matrix.remove(e);
   }
   public void RemoveCol(Integer idx)
   {
      saveSnapshot();
      --width;
      ArrayList<BMEntry> removeArrayList = new ArrayList<>();
      for (BMEntry e : matrix)
      {
         if (e.cIdx == idx)
         {
            removeArrayList.add(e);
         }
         else if (e.cIdx > idx)
         {
            --e.cIdx;
         }
      }
      for (BMEntry e : removeArrayList)
         matrix.remove(e);
   }
   private void saveSnapshot()
   {
      matrixhistory.push(new ArrayList<>(matrix));
      ignorerowshistory.push(new ArrayList<>(ignorerows));
      ignorecolshistory.push(new ArrayList<>(ignorecols));
      heighthistory.push(height);
      widthhistory.push(width);
   }
   public Integer[][] GetMaximalConf()
   {
      // This will return a configuration with all possible 0s filled according to the existing 1s (with no implication)
      // This means all the remaining blanks are 0-implications and some of them are also 1-implications
      Integer[][] ret = GetFullMatrix();
      boolean[][] zeroimp = new boolean[height][width];
      boolean[][] h1map = new boolean[width][width];//12
      boolean[][] h2map = new boolean[width][width];//21
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            for (int k = j + 1; k < width; ++k)
            {
               if (ret[i][j] == 1 && ret[i][k] == 2)
               {
                  h1map[j][k] = true;
               }
               else if (ret[i][j] == 2 && ret[i][k] == 1 && h1map[j][k])
               {
                  zeroimp[i][j] = true;
               }
            }
      for (int i = height - 1; i >= 0; --i)
         for (int j = 0; j < width; ++j)
            for (int k = j + 1; k < width; ++k)
            {
               if (ret[i][j] == 2 && ret[i][k] == 1)
               {
                  h2map[j][k] = true;
               }
               else if (ret[i][j] == 1 && ret[i][k] == 2 && h2map[j][k])
               {
                  zeroimp[i][k] = true;
               }
            }
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            if (!zeroimp[i][j] && ret[i][j] == 2)
      ret[i][j] = 0;
      return ret;
   }
   public double APRRatioDRFfillings() {
      boolean[][] ignm = GetFullIgnoreMap();
      Integer[][] m = GetFullMatrix();
      int numberofvars = 0;
      for(int i = 0; i<height; ++i)
         for(int j = 0; j<width; ++j)
            if (m[i][j] == 2)
               numberofvars++;
      FourDNF dnf = convertToDNF(ignm, m);
      return 1.0 - dnf.ApproximateRatio();
   }
   public long CountDRFfillings() throws InterruptedException {
      boolean[][] ignm = GetFullIgnoreMap();
      Integer[][] m = GetFullMatrix();
      int numberofvars = 0;
      for(int i = 0; i<height; ++i)
         for(int j = 0; j<width; ++j)
            if (m[i][j] == 2)
               numberofvars++;
      if (numberofvars >= 40)
         return -1;
      FourDNF dnf = convertToDNF(ignm, m);
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      if (numberofvars == 0) tmppower = 0;
      return tmppower - dnf.Count();
   }
   public double RatioDRFfillings() throws InterruptedException {
      boolean[][] ignm = GetFullIgnoreMap();
      Integer[][] m = GetFullMatrix();
      int numberofvars = 0;
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            if (m[i][j] == 2)
               numberofvars++;
      if (numberofvars >= 40)
         return -1.0;
      FourDNF dnf = convertToDNF(ignm, m);
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      return (double)(tmppower - dnf.Count())/(double)tmppower;
   }
   public long CountDRFfillingsOfMaxConf() throws InterruptedException {
      boolean[][] ignm = GetFullIgnoreMap();
      Integer[][] m = GetMaximalConf();
      int numberofvars = 0;
      for(int i = 0; i<height; ++i)
         for(int j = 0; j<width; ++j)
            if (m[i][j] == 2)
      numberofvars++;
      if (numberofvars >= 40)
         return -1;
      FourDNF dnf = convertToDNF(ignm, m);
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      if (numberofvars == 0) tmppower = 0;
      return tmppower - dnf.Count();
   }
   public double RatioDRFfillingsOfMaxConf() throws InterruptedException {
      boolean[][] ignm = GetFullIgnoreMap();
      Integer[][] m = GetMaximalConf();
      int numberofvars = 0;
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
            if (m[i][j] == 2)
      numberofvars++;
      if (numberofvars >= 40)
         return -1.0;
      FourDNF dnf = convertToDNF(ignm, m);
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      return (double)(tmppower - dnf.Count())/(double)tmppower;
   }
   private FourDNF convertToDNF(boolean[][] ignm, Integer[][] m)
   {
      FourDNF ret = new FourDNF();
      //index of vars
      Integer[][] ids = new Integer[height][width];
      Integer c = 0;
      for (int i = 0; i < height; ++i)
         for (int j = 0; j < width; ++j)
         {
            ids[i][j] = -1;
            if(!ignm[i][j])
            {
               if (m[i][j] == 2)
               {
                  ids[i][j] = c;
                  ++c;
               }
            }
         }

      for(int i = 0; i<height; ++i)
         for(int j = 0; j<width; ++j)
         {
            if(ignm[i][j])
            continue;
            for(int k = i+1; k<height; ++k)
               for(int l = j + 1; l<width; ++l)
               {
                  if (ignm[k][l] || ignm[i][l]|| ignm[k][j])
                  continue;
                  // [ij][il]
                  // [kj][kl]
                  if(m[i][j] != 0 && m[i][l] != 1 && m[k][j] != 1 && m[k][l] != 0)
                  {
                     Clause clause = new Clause();
                     if(m[i][j] == 2)
                     {
                        clause.literals.add(new Literal(ids[i][j], false));
                     }
                     if(m[i][l] == 2)
                     {
                        clause.literals.add(new Literal(ids[i][l], true));
                     }
                     if(m[k][j] == 2)
                     {
                        clause.literals.add(new Literal(ids[k][j], true));
                     }
                     if(m[k][l] == 2)
                     {
                        clause.literals.add(new Literal(ids[k][l], false));
                     }
                     ret.clauses.add(clause);
                  }
               }
         }
      return ret;
   }
   public void Revert()
   {
      if (matrixhistory.size() <= 0) return;
      matrix = matrixhistory.pop();
      ignorerows = ignorerowshistory.pop();
      ignorecols = ignorecolshistory.pop();
      height = heighthistory.pop();
      width = widthhistory.pop();
   }
   //Stats
   public Integer Height()
   {
      return height;
   }
   public Integer Width()
   {
      return width;
   }
}
