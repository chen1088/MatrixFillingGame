package com.applications;

import javax.swing.*;

public class WorkerStatAPRIS extends MFGSwingWorker{

   public WorkerStatAPRIS(BMContainer c) {
      super(c);
   }
   public WorkerStatAPRIS(BMContainer c, JTextPane tf, JProgressBar pb, JCheckBox cb,JPanel rp)
   {
      super(c,tf,pb,cb,rp);
   }

   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatAPRIS(container,resultField,progressBar,enable, subPanel);
   }

   @Override
   public void SetAppearance() {
      enable.setText("APRIS");
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
      return String.format("%.10f",1.0 - dnf.ApproximateRatio());//Importance Sampling performs worse.
   }
}
