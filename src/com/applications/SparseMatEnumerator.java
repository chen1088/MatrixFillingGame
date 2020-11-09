package com.applications;

import java.util.ArrayList;

public class SparseMatEnumerator {
   private int k;
   private int[][] mat;
   private int rightmost;
   public boolean IsEnd;
   public SparseMatEnumerator(Integer sparsity)
   {
      IsEnd = false;
      k = sparsity;
      rightmost = k - 1;
      mat = new int[k][2];
      for(int i = 0;i<k;++i)
      {
         mat[i][1] = i;
      }
      
   }
   public int[][] Next2()
   {
      
      return mat;
   }
   public int[][] Next()
   {
      boolean[] colflag = new boolean[rightmost+1];
      boolean[] colflag2 = new boolean[rightmost+1];
      int emptycol = rightmost+1;
      int emptycol2 = rightmost+1;
      int lastrow = -1;
      int p = -1;
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
            if(mat[i][0] != lastrow + 1 || mat[i][1] != lastcol)
            {
               p = i;
            }
         }
         else
         {
            // can fill any columns
            if(mat[i][0] != lastrow + 1 || mat[i][1] != rightmost)
            {
               p = i;
            }
         }
         // fill the entry
         if(!colflag[mat[i][1]])
         {
            colflag[mat[i][1]] = true;
            emptycol--;
         }
         lastrow = mat[i][0];
      }

      if(p == -1)
      {
         if(rightmost == 0)
         {
            IsEnd = true;
            return mat;
         }
         // this happens if the configuration is the last one of rightmost n
         rightmost--;
         for(int i = 0;i<k;++i)
         {
            mat[i][0] = i/(rightmost+1);
            mat[i][1] = i%(rightmost+1);
         }
         return mat;
      }

      for(int i = 0;i<p;++i)
      {
         if(!colflag2[mat[i][1]]) {
            colflag2[mat[i][1]] = true;
            emptycol2--;
         }
      }
      int lastrow2 = -1;
      int lastcol2 = -1;
      if(emptycol2 == k-p)
      {
         boolean added = false;
         int c = mat[p][1];
         int r = mat[p][0];
         for(int j = c+1;j<rightmost+1;++j)
         {
            if(!colflag2[j])
            {
               mat[p][0] = r;
               mat[p][1] = j;
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
                  mat[p][0] = r+1;
                  mat[p][1] = j;
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
         int c = mat[p][1];
         int r = mat[p][0];
         if(c == rightmost)
         {
            mat[p][0] = r+1;
            mat[p][1] = 0;
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
            mat[p][0] = r;
            mat[p][1] = c+1;
            if(!colflag2[c+1])
            {
               colflag2[c+1] = true;
               emptycol2--;
            }
            lastrow2 = r;
            lastcol2 = c+1;
         }
      }
      for(int i = p+1;i<k;++i)
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
                  mat[i][0] = r;
                  mat[i][1] = j;
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
                  mat[i][0] = r+1;
                  mat[i][1] = j;
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
            if(c == rightmost)
            {
               lastcol2 = 0;
               lastrow2 = r+1;
               mat[i][0] = r+1;
               mat[i][1] = 0;
               if(!colflag2[0])
               {
                  colflag2[0] = true;
                  emptycol2--;
               }
            }
            else
            {
               lastcol2 = c+1;
               lastrow2 = r;
               mat[i][0] = r;
               mat[i][1] = c+1;
               if(!colflag2[c+1])
               {
                  colflag2[c+1] = true;
                  emptycol2--;
               }
            }
         }
      }
      return mat;
   }
   public static int[][] SparseToFull(int[][] m)
   {
      int r = 0;
      int c = 0;
      for(int i = 0;i<m.length;++i)
      {
         r = Math.max(m[i][0],r);
         c = Math.max(m[i][1],c);
      }
      int h = r+1;
      int w = c+1;
      int[][] ret = new int[h][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            ret[i][j] = 2;
      for(int i = 0;i<m.length;++i)
      {
         int[] e = m[i];
         ret[e[0]][e[1]] = 1;
      }
      return ret;
   }
   public static void Display(int[][] m)
   {
      int r = 0;
      int c = 0;
      for(int i = 0;i<m.length;++i)
      {
         r = Math.max(m[i][0],r);
         c = Math.max(m[i][1],c);
      }
      r++;
      c++;
      boolean[][] ret = new boolean[r][c];
      for(int i = 0;i<m.length;++i)
      {
         int[] e = m[i];
         ret[e[0]][e[1]] = true;
      }
      for(int i = 0;i<r;++i) {
         for (int j = 0; j < c; ++j) {
            System.out.print((ret[i][j]?'1':' ') + " ");
         }
         System.out.print("\n");
      }
   }
   public int[] Score()
   {
      int[] ret = new int[2];
      int h = mat[k-1][0] + 1;
      int w = rightmost + 1;
      int[][] dense = new int[h][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            dense[i][j] = 2;
      for(int i = 0;i<k;++i)
      {
         dense[mat[i][0]][mat[i][1]] = 1;
      }

      boolean[][] zeroimp = new boolean[h][w];
      boolean[][] g1map = new boolean[w][w];//12
      boolean[][] g2map = new boolean[w][w];//21
      for (int i = 0; i < h; ++i)
         for (int j = 0; j < w; ++j)
            for (int k = j + 1; k < w; ++k)
            {
               if (dense[i][j] == 1 && dense[i][k] == 2)
               {
                  g1map[j][k] = true;
               }
               else if (dense[i][j] == 2 && dense[i][k] == 1 && g1map[j][k])
               {
                  zeroimp[i][j] = true;
               }
            }
      for (int i = h - 1; i >= 0; --i)
         for (int j = 0; j < w; ++j)
            for (int k = j + 1; k < w; ++k)
            {
               if (dense[i][j] == 2 && dense[i][k] == 1)
               {
                  g2map[j][k] = true;
               }
               else if (dense[i][j] == 1 && dense[i][k] == 2 && g2map[j][k])
               {
                  zeroimp[i][k] = true;
               }
            }
      for (int i = 0; i < h; ++i)
         for (int j = 0; j < w; ++j)
            if (!zeroimp[i][j] && dense[i][j] == 2)
               dense[i][j] = 0;
      //
      int Bcount = 0;
      int B0count = 0;
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(dense[i][j] == 2)
               Bcount++;
      boolean[][] oneimpmap = new boolean[h][w];
      boolean[][] h1map = new boolean[w][w];
      boolean[][] h2map = new boolean[w][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            for(int k = j+1;k<w;++k)
            {
               if(dense[i][j] == 2 && dense[i][k] == 0)
                  h1map[j][k] = true;
               else if(dense[i][j] == 0 && dense[i][k] == 2 && h1map[j][k])
                  oneimpmap[i][k] = true;
            }
      for(int i = h - 1;i>=0;--i)
         for(int j = 0;j<w;++j)
            for(int k = j+1;k<w;++k)
            {
               if(dense[i][j] == 0 && dense[i][k] == 2)
                  h2map[j][k] = true;
               else if(dense[i][j] == 2 && dense[i][k] == 0 && h2map[j][k])
                  oneimpmap[i][j] = true;
            }
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(oneimpmap[i][j])
               B0count++;
      ret[0] = B0count;
      ret[1] = Bcount;
      return ret;
   }
   public int ScoreNew()
   {
      //assume that the ratio is 1
      int ret = 0;

      return ret;
   }
   public static int EditDistance(int[][] p,int[][] q)
   {
      int ret = 0;

      return ret;
   }
   public static void main(String[] args){
      SparseMatEnumerator e = new SparseMatEnumerator(9);
      int count = 0;
      double maxratio = 0.0;
      while(!e.IsEnd)
      {
         int[] tmp = e.Score();
         double ratio = (double)tmp[0]/tmp[1];
         if(ratio>maxratio)
         {
            maxratio = ratio;
            Display(e.mat);
            System.out.printf("ratio: %f\n",ratio);
         }
         count++;
         //Display(e.mat);
         //System.out.print("\n");
         e.Next();
      }
      System.out.printf("count: %d\n",count);
   }
}
