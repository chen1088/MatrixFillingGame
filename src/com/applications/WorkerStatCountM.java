package com.applications;

import javax.swing.*;

public class WorkerStatCountM extends MFGSwingWorker{
   public WorkerStatCountM(BMContainer c) {
      super(c);
   }
   public WorkerStatCountM(BMContainer c, JTextPane tf, JProgressBar pb, JCheckBox cb,JPanel rp)
   {
      super(c,tf,pb,cb,rp);
   }
   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatCountM(container,resultField,progressBar,enable, subPanel);
   }

   @Override
   public void SetAppearance() {
      enable.setText("CountOfM");
   }

   @Override
   protected String doInBackground() {
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
         return "-1";
      FourDNF dnf = container.ConvertToDNF(ignm, m);
      dnf.caller = this;
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      if (numberofvars == 0) tmppower = 0;
      return String.format("%d",tmppower - dnf.Count());
   }
}

