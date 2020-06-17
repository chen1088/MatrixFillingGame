package com.applications;

import javax.swing.*;

public class WorkerStatCountM extends MFGSwingWorker{
   public WorkerStatCountM(BMContainer c, JTextField tf, JProgressBar pb) {
      super(c, tf, pb);
   }

   @Override
   protected Object doInBackground() throws Exception {
      boolean[][] ignm = container.GetFullIgnoreMap();
      Integer[][] m = container.GetMaximalConf();
      int numberofvars = 0;
      int h = container.Height();
      int w = container.Width();
      for(int i = 0; i<h; ++i)
         for(int j = 0; j<w; ++j)
            if (m[i][j] == 2)
               numberofvars++;
      if (numberofvars >= 40)
         return -1;
      FourDNF dnf = container.ConvertToDNF(ignm, m);
      dnf.caller = this;
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      if (numberofvars == 0) tmppower = 0;
      return tmppower - dnf.Count();
   }
}
