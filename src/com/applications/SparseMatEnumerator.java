package com.applications;

import java.util.ArrayList;

public class SparseMatEnumerator {
   private Integer k;
   private ArrayList<Integer[]> mat;
   private Integer rightmost;
   public boolean IsEnd;
   public SparseMatEnumerator(Integer sparsity)
   {
      IsEnd = false;
      k = sparsity;
      rightmost = k - 1;
//      rightmost = 1;
      mat = new ArrayList<>();
      for(int i = 0;i<k;++i)
      {
         Integer[] e = new Integer[2];
         e[0] = 0;
         e[1] = i;
         mat.add(e);
      }
//      Integer[] e = new Integer[2];
//      Integer[] e1 = new Integer[2];
//      Integer[] e2 = new Integer[2];
//      e[0] = 0;e[1] = 0;
//      e1[0] = 1;e1[1] = 0;
//      e2[0] = 1;e2[1] = 1;
//      mat.add(e);
//      mat.add(e1);
//      mat.add(e2);
   }
   public ArrayList<Integer[]> Next()
   {
      ArrayList<Integer[]> ret = new ArrayList<>();
      boolean[] colflag = new boolean[rightmost+1];
      boolean[] colflag2 = new boolean[rightmost+1];
      int emptycol = rightmost+1;
      int emptycol2 = rightmost+1;
      int lastrow = -1;
      int entryofmodification = -1;
      for(int i = 0;i<k;++i)
      {
         if(emptycol == k-i)
         {
            // forced to fill in empty cols
            int lastcol = -1;
            for(int j = rightmost;j>=0;--j)
            {
               if(!colflag[j])
               {
                  lastcol = j;
                  break;
               }
            }
            if(mat.get(i)[0] != lastrow + 1 || mat.get(i)[1] != lastcol)
            {
               entryofmodification = i;
            }
         }
         else
         {
            // can fill any columns
            if(mat.get(i)[0] != lastrow + 1 || mat.get(i)[1] != rightmost)
            {
               entryofmodification = i;
            }
         }
         // fill the entry
         if(!colflag[mat.get(i)[1]])
         {
            colflag[mat.get(i)[1]] = true;
            emptycol--;
         }
         lastrow = mat.get(i)[0];
      }

      if(entryofmodification == -1)
      {
         if(rightmost == 0)
         {
            IsEnd = true;
            return mat;
         }
         // this happens if the configuration is the last one of rightmost n
         mat.clear();
         rightmost--;
         for(int i = 0;i<k;++i)
         {
            Integer[] e = new Integer[2];
            e[0] = i / (rightmost + 1);
            e[1] = i % (rightmost + 1);
            mat.add(e);
         }
         return mat;
      }

      for(int i = 0;i<entryofmodification;++i)
      {
         ret.add(mat.get(i));
         if(!colflag2[mat.get(i)[1]]) {
            colflag2[mat.get(i)[1]] = true;
            emptycol2--;
         }
      }
      int lastrow2 = -1;
      int lastcol2 = -1;
      if(emptycol2 == k-entryofmodification)
      {
         boolean added = false;
         int c = mat.get(entryofmodification)[1];
         int r = mat.get(entryofmodification)[0];
         for(int j = c+1;j<rightmost+1;++j)
         {
            if(!colflag2[j])
            {
               Integer[] e = new Integer[2];
               e[0] = r;
               e[1] = j;
               ret.add(e);
               colflag2[j] = true;
               emptycol2--;
               added = true;
               lastrow2 = r;
               lastcol2 = j;
               break;
            }
         }
         if(!added)
         {
            for(int j = 0;j<c+1;++j)
            {
               if(!colflag2[j])
               {
                  Integer[] e = new Integer[2];
                  e[0] = r + 1;
                  e[1] = j;
                  ret.add(e);
                  colflag2[j] = true;
                  emptycol2--;
                  lastrow2 = r + 1;
                  lastcol2 = j;
                  break;
               }
            }
         }
      }
      else
      {
         int c = mat.get(entryofmodification)[1];
         int r = mat.get(entryofmodification)[0];
         Integer[] e = new Integer[2];
         if(c == rightmost)
         {
            e[0] = r+1;
            e[1] = 0;
            ret.add(e);
            if(!colflag2[0])
            {
               colflag2[0] = true;
               emptycol2--;
            }
            lastrow2 = r+1;
            lastcol2 = 0;
         }
         else
         {
            e[0] = r;
            e[1] = c+1;
            ret.add(e);
            if(!colflag2[c+1])
            {
               colflag2[c+1] = true;
               emptycol2--;
            }
            lastrow2 = r;
            lastcol2 = c+1;
         }
      }
      for(int i = entryofmodification+1;i<k;++i)
      {
         int c = lastcol2;
         int r = lastrow2;
         if(emptycol2 == k-i)
         {
            // need to find the next empty col
            boolean added = false;
            for(int j = c+1;j<rightmost+1;++j)
            {
               if(!colflag2[j])
               {
                  Integer[] e = new Integer[2];
                  e[0] = r;
                  e[1] = j;
                  ret.add(e);
                  colflag2[j] = true;
                  emptycol2--;
                  lastrow2 = r;
                  lastcol2 = j;
                  added = true;
                  break;
               }
            }
            if(added)
               continue;
            for(int j = 0;j<c+1;++j)
            {
               if(!colflag2[j])
               {
                  Integer[] e = new Integer[2];
                  e[0] = r + 1;
                  e[1] = j;
                  ret.add(e);
                  lastcol2 = j;
                  lastrow2 = r+1;
                  colflag2[j] = true;
                  emptycol2--;
                  break;
               }
            }
         }
         else
         {
            Integer[] e = new Integer[2];
            if(c == rightmost)
            {
               e[0] = r+1;
               e[1] = 0;
               lastcol2 = 0;
               lastrow2 = r+1;
               ret.add(e);
               if(!colflag2[0])
               {
                  colflag2[0] = true;
                  emptycol2--;
               }
            }
            else
            {
               e[0] = r;
               e[1] = c+1;
               lastcol2 = c+1;
               lastrow2 = r;
               ret.add(e);
               if(!colflag2[c+1])
               {
                  colflag2[c+1] = true;
                  emptycol2--;
               }
            }
         }
      }
      mat = ret;
      return ret;
   }
   public static boolean[][] SparseToFull(ArrayList<Integer[]> m)
   {
      int r = 1;
      int c = 1;
      for(int i = 0;i<m.size();++i)
      {
         r = Math.max(m.get(i)[0],r);
         c = Math.max(m.get(i)[1],c);
      }
      boolean[][] ret = new boolean[r][c];
      for(int i = 0;i<m.size();++i)
      {
         Integer[] e = m.get(i);
         ret[e[0]][e[1]] = true;
      }
      return ret;
   }
   public static void Display(ArrayList<Integer[]> m)
   {
      int r = 0;
      int c = 0;
      for(int i = 0;i<m.size();++i)
      {
         r = Math.max(m.get(i)[0],r);
         c = Math.max(m.get(i)[1],c);
      }
      r++;
      c++;
      boolean[][] ret = new boolean[r][c];
      for(int i = 0;i<m.size();++i)
      {
         Integer[] e = m.get(i);
         ret[e[0]][e[1]] = true;
      }
      for(int i = 0;i<r;++i) {
         for (int j = 0; j < c; ++j) {
            System.out.print((ret[i][j]?'1':'0') + " ");
         }
         System.out.print("\n");
      }
   }
   public static void main(String[] args){
      SparseMatEnumerator e = new SparseMatEnumerator(8);
      int count = 0;
      while(!e.IsEnd)
      {
         count++;
         //Display(e.mat);
         //System.out.print("\n");
         e.Next();
      }
      System.out.printf("count: %d\n",count);
   }
}
