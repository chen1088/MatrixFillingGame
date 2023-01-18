package framework.combinatorics;

import java.util.*;

public class DFA {
   public int[][] transfunc;
   public boolean[] accstates;
   public int[][] countingstats;
   public boolean trimmed;
   public int alphabetsize;
   public int statecount;
   public DFA()
   {
      transfunc = null;
      accstates = null;
      trimmed = true;
      alphabetsize = 0;
      statecount = 0;
   }
   public DFA(int scount, int asize)
   {
      transfunc = new int[scount][asize];
      accstates = new boolean[scount];
      trimmed = false;
      alphabetsize = asize;
      statecount = scount;
   }
   public DFA(int[][] tf, boolean[] as, boolean t)
   {
      transfunc = tf;
      accstates = as;
      trimmed = t;
      alphabetsize = tf[0].length;
      statecount = tf.length;
   }
   public boolean Evaluate(int[] input)
   {
      int currentstate = 0;
      for(int i: input)
      {
         currentstate = transfunc[currentstate][i];
      }
      return accstates[currentstate];
   }
   public void SetTransition(Transition tran)
   {
      int m = Integer.max(tran.statep, tran.stateq);
      if(m>=transfunc.length)
      {
         int[][] transfuncnew = new int[m+1][alphabetsize];
         for(int i = 0;i< statecount;++i)
            for(int j = 0;j<alphabetsize;++j)
               transfuncnew[i][j] = transfunc[i][j];
         transfunc = transfuncnew;
         statecount = m+1;
         trimmed = false;
      }
      transfunc[tran.statep][tran.alpha] = tran.stateq;
   }
   public void SetTransitions(Transition[] trans)
   {
      for(Transition t:trans)
      {
         SetTransition(t);
      }
   }
   public ArrayList<Transition> getTransitions()
   {
      ArrayList<Transition> ret = new ArrayList<>();
      for(int i = 0;i<statecount;++i)
         for(int j = 0;j<alphabetsize;++j)
            ret.add(new Transition(i,transfunc[i][j],j));
      return ret;
   }
   public void SetAcceptingState(int state, boolean isacc)
   {
      if(state >= accstates.length)
      {
         boolean[] newaccstates = new boolean[state+1];
         for(int i = 0;i<accstates.length;++i)
            newaccstates[i] = accstates[i];
         accstates = newaccstates;
      }
      accstates[state] = isacc;
   }
   public boolean IsAccepting(int state)
   {
      if(state>=accstates.length) return false;
      else return accstates[state];
   }

   public DFA Trim()
   {
      if(trimmed) return this;
      //BFS all states
      Queue<Integer> queue = new LinkedList<>();
      queue.add(0);
      boolean[] reachable = new boolean[statecount];
      reachable[0] = true;
      while(!queue.isEmpty())
      {
         int current = queue.poll();
         int[] nextstates = transfunc[current];
         for(int i = 0;i<alphabetsize;++i)
         {
            if(!reachable[nextstates[i]])
            {
               reachable[nextstates[i]] = true;
               queue.add(nextstates[i]);
            }
         }
      }
      // redirect the state indices
      int[] idxmap = new int[statecount];
      int ridx = 0;
      for(int i = 0;i<statecount;++i)
      {
         if(reachable[i])
         {
            idxmap[i] = ridx;
            ridx++;
         }
         else
         {
            idxmap[i] = -1;
         }
      }
      // copy all reachable states
      int[][] newtrans = new int[ridx][alphabetsize];
      boolean[] newacc = new boolean[ridx];
      for(int i = 0;i< statecount;++i)
         for (int j = 0; j < alphabetsize; ++j){
            if(!reachable[i]) continue;
            newtrans[idxmap[i]][j] = idxmap[transfunc[i][j]];
            newacc[idxmap[i]] = IsAccepting(i);
         }
      return new DFA(newtrans,newacc,true);
   }
   public DFA Minimize()
   {
      if(!trimmed) {
         DFA temp = Trim();
         return temp.Minimize();
      }
      // Hopcroft's DFA minimization
      // get the invmap for the dfa first
      // the invert states of a state A on input x is stored in invmap[A][x]
      Set<Integer>[][] invmap = GetInvMap();
      int maxclass = 2;

      // for partition_table
      final int prev = 0;
      final int next = 1;
      final int classnum = 2;

      // for class_table
      final int headeridx = 0;
      final int classsize = 1;
      final int classcount = 2;

      int[][] partition_table = new int[statecount][3];
      int[][] class_table = new int[statecount][3];
      LinkedList<int[]> waitinglist = new LinkedList<>();
      boolean[][] memberofwl = new boolean[statecount][alphabetsize];

      boolean hasaccstates = false;
      boolean hasrejstates = false;
      for(int[] r:class_table)
         Arrays.fill(r,-1);
      for(int[] r:partition_table)
         Arrays.fill(r,-1);
      // init the partition table and class_table
      int prevrej = -1;
      int prevacc = -1;
      for(int i = 0;i<statecount;++i)
         if(accstates[i])
         {
            hasaccstates = true;
            partition_table[i][prev] = prevacc;
            partition_table[i][next] = -1;
            partition_table[i][classnum] = 0;
            if(prevacc != -1)
            {
               partition_table[prevacc][next] = i;
            }
            else
            {
               class_table[0][headeridx] = i;
               class_table[0][classsize] = 0;
               class_table[0][classcount] = 0;
            }
            class_table[0][classsize]++;
            prevacc = i;
         }
         else
         {
            hasrejstates = true;
            partition_table[i][prev] = prevrej;
            partition_table[i][next] = -1;
            partition_table[i][classnum] = 1;
            if(prevrej != -1)
            {
               partition_table[prevrej][next] = i;
            }
            else
            {
               class_table[1][headeridx] = i;
               class_table[1][classsize] = 0;
               class_table[1][classcount] = 0;
            }
            class_table[1][classsize]++;
            prevrej = i;
         }
      // init the waiting list
      if(!hasaccstates&&!hasrejstates)
      {
         // return an empty dfa
         return new DFA();
      }
      if(!hasaccstates)
      {
         // return a dfa with one rej state
         DFA ret = new DFA(1,alphabetsize);
         ret.SetAcceptingState(0,false);
         return ret;
      }
      if(!hasrejstates)
      {
         // return a dfa with one acc state
         DFA ret = new DFA(1,alphabetsize);
         ret.SetAcceptingState(0,true);
         return ret;
      }
      // init the waiting list
      for(int i = 0;i<alphabetsize;++i)
      {
         waitinglist.push(new int[]{0,i});
         waitinglist.push(new int[]{1,i});
      }
      while(!waitinglist.isEmpty())
      {
         int[] A = waitinglist.poll();//.poll() will remove the element
         int x = A[0];// class
         int c = A[1];// alphabet
         memberofwl[x][c] = false;// removed from waitinglist.
         Map<Integer, Set<Integer>> suspects = new HashMap<>();
         // collect the suspicous classes and states
         int hidx = class_table[x][headeridx];
         //int size = class_table[x][classsize];
         int i = hidx;
         while(i!= -1)
         {
            Set<Integer> intereststates = invmap[i][c];
            if(intereststates!=null)
            {
               for(int intereststate:intereststates)
               {
                  int interestclass = partition_table[intereststate][classnum];
                  if(!suspects.containsKey(interestclass))
                  {
                     Set<Integer> hs = new HashSet<>();
                     hs.add(intereststate);
                     suspects.put(interestclass, hs);
                  }
                  else
                  {
                     Set<Integer> temp = suspects.get(interestclass);
                     temp.add(intereststate);
                  }
               }
            }
            i = partition_table[i][next];
         }
         // refine the partition table
         Set<Integer> susclass = suspects.keySet();
         for(int sc:susclass)
         {
            Set<Integer> susstates = suspects.get(sc);
            if(susstates.size()>=class_table[sc][classsize])
               continue; // this class is not split.

            int lastsplitstate = -1;
            for(int ss: susstates)
            {
               int ns = partition_table[ss][next];
               int ps = partition_table[ss][prev];
               if(partition_table[ss][prev]==-1)
               {
                  // the first one of the old class gets split, we need to update the class header
                  class_table[sc][headeridx] = ns;
                  partition_table[ns][prev] = -1;// here ns cannot be -1 otherwise susstates.size = class.size
               }
               else
               {
                  // if this is not the first one of the old class, we just update the link
                  if(lastsplitstate!=-1)
                  {
                     // if this is not the first one of the new class
                     partition_table[lastsplitstate][next] = ss;
                  }
                  else
                  {
                     // if this is the first one of the new class
                     class_table[maxclass][headeridx] = ss;
                  }
                  partition_table[ss][prev] = lastsplitstate;
                  partition_table[ps][next] = ns;
                  if(ns!=-1)
                  {
                     // if this state is not the last one of the class
                     partition_table[ns][prev] = ps;
                  }
               }
               partition_table[ss][classnum] = maxclass;// assign the new class number
               partition_table[ss][next] = -1;
               lastsplitstate = ss;
            }
            class_table[sc][classsize]-=susstates.size();
            class_table[maxclass][classsize] = susstates.size();
            // add the class to waiting list
            for(int a = 0;a<alphabetsize;++a)
            {
               if(memberofwl[sc][a])
               {
                  // add new class
                  memberofwl[maxclass][a] = true;
                  waitinglist.add(new int[]{maxclass,a});
               }
               else
               {
                  // add the smaller of the new and old class
                  // NOTICE this is the version without enhancement
                  if(class_table[sc][classsize] <= class_table[maxclass][classsize])
                  {
                     memberofwl[sc][a] = true;
                     waitinglist.add(new int[]{sc,a});
                  }
                  else
                  {
                     memberofwl[maxclass][a] = true;
                     waitinglist.add(new int[]{maxclass,a});
                  }
               }
            }
            maxclass++;
         }

      }
      // convert the partition to the new DFA
      // compute the class representative
      int[] representatives = new int[maxclass];
      Arrays.fill(representatives,-1);
      representatives[partition_table[0][classnum]] = 0;// state 0 is always the start state it is always the representative of its class
      for(int i = 0;i<statecount;++i)
      {
         if(representatives[partition_table[i][classnum]] == -1)
         {
            representatives[partition_table[i][classnum]] = i;
         }
      }
      for(int i = 0;i<statecount;++i)
         for(int j = 0;j<alphabetsize;++j)
         {
            transfunc[i][j] = representatives[partition_table[transfunc[i][j]][classnum]];
         }
      trimmed = false;
      return Trim();
   }
   public Set<Integer>[][] GetInvMap()
   {
      Set<Integer>[][] ret = new HashSet[statecount][alphabetsize];
      for(int i = 0;i<statecount;++i)
         for(int j = 0;j<alphabetsize;++j) {
            if (ret[transfunc[i][j]][j] == null)
               ret[transfunc[i][j]][j] = new HashSet<>();
            ret[transfunc[i][j]][j].add(i);
         }
      return ret;
   }
   public DFA Intersects(DFA other)
   {
      LinkedList<int[]> queue = new LinkedList<>();
      Set<Integer> visited = new HashSet<>();
      DFA ret = new DFA(1,2);
      visited.add(0);
      queue.add(new int[]{0,0});
      while(!queue.isEmpty())
      {
         int[] pair = queue.poll();
         int p1 = pair[0];
         int p2 = pair[1];
         int p1p2 = Util.PairingFunction(p1,p2);
         if(accstates[p1] && other.accstates[p2])
            ret.SetAcceptingState(p1p2,true);
         for(int i = 0;i<alphabetsize;++i)
         {
            int q1 = transfunc[pair[0]][i];
            int q2 = other.transfunc[pair[1]][i];
            int q1q2 = Util.PairingFunction(q1,q2);
            ret.SetTransition(new Transition(p1p2,q1q2,i));
            if(!visited.contains(q1q2))
            {
               visited.add(q1q2);
               queue.add(new int[]{q1,q2});
            }
         }
      }
      ret.statecount = ret.transfunc.length;
      ret = ret.Trim();
      return ret;
   }
   public int[][] ComputeCountStats(int step)
   {
      if(step<=0) return null;
      countingstats = new int[statecount][step];
      Set<Integer>[][] invmap = GetInvMap();
      countingstats[0][0] = 1;
      for(int i = 1;i<step;++i)
         for(int j = 0;j<statecount;++j)
         {
            Set<Integer>[] invstatesall = invmap[j];
            for(int k = 0;k<alphabetsize;++k)
            {
               Set<Integer> invstates = invstatesall[k];
               if(invstates == null) continue;
               for(int p : invstates)
               {
                  countingstats[j][i] += countingstats[p][i-1];
               }
            }
         }
      return countingstats;
   }
   public DFA IntersectsV2(DFA other)
   {

      return this;
   }

   // helper functions

}
