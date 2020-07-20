package com.applications;

import java.util.Stack;

public class BMUtil {
   public static boolean HasDiagRect_naive(Integer[][] m,int h, int w)
   {
      boolean[][] pair10 = new boolean[w][w];
      for(int i = 0;i<h;++i)
      {
         for(int j = 0;j<w-1;++j)
            for(int k = j+1;k<w;++k)
            {
               if(m[i][j] == 0 && m[i][k] == 1 && pair10[j][k])
                  return true;
               else if(m[i][j] == 1 && m[i][k] == 0)
                  pair10[j][k] = true;
            }
      }
      return false;
   }
   public static boolean HasDiagRect_mike(Integer[][] m,int h, int w)
   {
      //TODO: Mike's divide and conquer algorithm
      return false;
   }
   public static boolean HasDiagRect_chen(Integer[][] m,int h, int w)
   {
      boolean[][][] state = new boolean[h][w][4];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
               if(m[i][j] == 0)
               {
                  state[i][j][1] = true;
                  state[i][j][3] = true;
               }
               else if(m[i][j] == 1)
               {
                  state[i][j][0] = true;
                  state[i][j][2] = true;
               }
      // DFS algorithm to get the convex hull
      Stack<Tuple<Integer,Integer>> ds = new Stack<>();
      boolean[][] visited = new boolean[h][w];
      boolean[][] onloop = new boolean[h][w];
      // DFS for every border entries
      //TODO: this DFS is going to look nasty
      return false;
   }

   public static Integer[][] GreedyFillMFG(Integer[][] mcfg, int h, int w)
   {
      // assume that mcfg is already maximum.
      Integer[][] ret = new Integer[h][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            ret[i][j] = mcfg[i][j];
      boolean[][] map20ud = new boolean[w][w];
      boolean[][] map02du = new boolean[w][w];
      // fill 0s
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w-1;++j)
            for(int k = j+1;k<w;++k)
            {
               if(mcfg[i][j] == 0 && mcfg[i][k] == 2 && map20ud[j][k])
                  ret[i][k] = 0;
               else if(mcfg[i][j] == 2 && mcfg[i][k] == 0)
                  map20ud[j][k] = true;
            }
      for(int i = h-1;i>=0;--i)
         for(int j = 0;j<w-1;++j)
            for(int k = j+1;k<w;++k)
            {
               if(mcfg[i][j] == 2 && mcfg[i][k] == 0 && map02du[j][k])
                  ret[i][j] = 0;
               else if(mcfg[i][j] == 0 && mcfg[i][k] == 2)
                  map02du[j][k] = true;
            }
      // fill 1s
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(ret[i][j] == 2)
               ret[i][j] = 1;
      return ret;
   }
   public static void main(String[] args){
      int h = 6;
      int w = 6;
      Integer[][] m = new Integer[h][w];
      boolean res = HasDiagRect_chen(m,h,w);
   }
}
