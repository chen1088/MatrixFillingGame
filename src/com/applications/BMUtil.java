package com.applications;

import java.util.LinkedList;
import java.util.Queue;
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
      //0 1
      //2 3
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
               if(m[i][j] == 0)
               {
                  state[i][j][1] = true;
                  state[i][j][2] = true;
               }
               else if(m[i][j] == 1)
               {
                  state[i][j][0] = true;
                  state[i][j][3] = true;
               }
      stateShaving(state,h,w);
      return false;

   }

   private static void stateShaving(boolean[][][] state, int h, int w)
   {
      // BFS algorithm to get the convex hull
      Queue<Triplet<Integer,Integer,Integer>> ds = new LinkedList<>();

      boolean[][][] visited = new boolean[h][w][4];
      boolean[][][] instack = new boolean[h][w][4];
      Tuple<Integer,Integer>[][][] ndiffcw = new Tuple[h][w][4];
      Tuple<Integer,Integer>[][][] nsame = new Tuple[h][w][4];
      //TODO: this DFS is going to look nasty
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            for(int k = 0;k<4;++k)
            {
               // Take one unvisited possible
               // Two bfs processes
               if(state[i][j][k] && !visited[i][j][k])
               {
                  switch (k)
                  {
                     case 0:

                        break;
                     case 1:
                        break;
                     case 2:
                        break;
                     case 3:
                        break;
                  }
                  // bfs the left branch
                  
                  // bfs the right branch
               }
            }
   }

   private static void dfs(boolean[][][] state, boolean[][][] visited, int h, int w, int i, int j, int s)
   {

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
   public static boolean HasBlank(Integer[][] cfg, int h, int w)
   {
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(cfg[i][j] == 2)
               return true;
      return false;
   }
   public static Integer[][] GreedyFillMFG2(Integer[][] mcfg, int h, int w)
   {
      // assume that mcfg is already maximum.
      Integer[][] ret = new Integer[h][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            ret[i][j] = mcfg[i][j];
      // from left to right up to bottom
      for(int j = 0;j<w;++j)
         for(int i = 0;i<h;++i)
         {
            if(ret[i][j] != 2) continue;
            // test if the blank is a forced blank
            if(is0forced(ret,h,w,i,j))
            {
               if(is1forced(ret,h,w,i,j))
               {
                  System.out.printf("bad position at%d,%d%n",i,j);
                  break;//This should not happen.
               }
               else
               {
                  ret[i][j] = 1;
                  continue;
               }
            }
            else
            {
               if(is1forced(ret,h,w,i,j))
               {
                  ret[i][j] = 0;
                  continue;
               }
            }
            // not a forced blank
            // test if the blank is a 1-implication (max conf blanks are all 0-implications)
            if(is1imp(ret,h,w,i,j))
               ret[i][j] = 0;
            else
               ret[i][j] = 1;
         }
      return ret;
   }
   // filling 1 will make a forced blank
   public static boolean is1imp(Integer[][] mcfg, int h, int w, int i, int j)
   {
      if(mcfg[i][j] != 2) return false;
      // [k l]  [k j]
      // [i l]  [i j]
      for(int k=0;k<i;++k)
         for(int l = 0;l<j;++l)
         {
            if(mcfg[k][l] == 2 && mcfg[i][l] == 0 && mcfg[k][j] == 0)
               return true;
            if(mcfg[k][l] == 1 && mcfg[i][l] == 0 && mcfg[k][j] == 2)
               return true;
            if(mcfg[k][l] == 1 && mcfg[i][l] == 2 && mcfg[k][j] == 0)
               return true;
         }
      // [i j]  [i l]
      // [k j]  [k l]
      for(int k=i+1;k<h;++k)
         for(int l = j+1;l<w;++l)
         {
            if(mcfg[k][l] == 2 && mcfg[i][l] == 0 && mcfg[k][j] == 0)
               return true;
            if(mcfg[k][l] == 1 && mcfg[i][l] == 0 && mcfg[k][j] == 2)
               return true;
            if(mcfg[k][l] == 1 && mcfg[i][l] == 2 && mcfg[k][j] == 0)
               return true;
         }
      return false;
   }
   // filling 1 will make a rectangle so we should fill 0
   public static boolean is1forced(Integer[][] mcfg, int h, int w, int i, int j)
   {
      if(mcfg[i][j] != 2) return false;
      // [k l]  [k j]
      // [i l]  [i j]
      for(int k=0;k<i;++k)
         for(int l = 0;l<j;++l)
         {
            if(mcfg[k][l] == 1 && mcfg[i][l] == 0 && mcfg[k][j] == 0)
               return true;
         }
      // [i j]  [i l]
      // [k j]  [k l]
      for(int k=i+1;k<h;++k)
         for(int l = j+1;l<w;++l)
         {
            if(mcfg[k][l] == 1 && mcfg[i][l] == 0 && mcfg[k][j] == 0)
               return true;
         }
      return false;
   }
   // filling 0 will make a rectangle so we should fill 1
   public static boolean is0forced(Integer[][] mcfg, int h, int w, int i, int j)
   {
      if(mcfg[i][j] != 2) return false;
      // [i l]  [i j]
      // [k l]  [k j]
      for(int k=i+1;k<h;++k)
         for(int l = 0;l<j;++l)
         {
            if(mcfg[k][l] == 0 && mcfg[i][l] == 1 && mcfg[k][j] == 1)
               return true;
         }
      // [k j]  [k l]
      // [i j]  [i l]
      for(int k=0;k<i;++k)
         for(int l = j+1;l<w;++l)
         {
            if(mcfg[k][l] == 0 && mcfg[i][l] == 1 && mcfg[k][j] == 1)
               return true;
         }
      return false;
   }
   public static void main(String[] args){
      int h = 6;
      int w = 6;
      Integer[][] m = new Integer[h][w];
      boolean res = HasDiagRect_chen(m,h,w);
   }
}
