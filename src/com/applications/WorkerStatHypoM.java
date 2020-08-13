package com.applications;

import javax.swing.*;

public class WorkerStatHypoM extends MFGSwingWorker{

   public WorkerStatHypoM(BMContainer c) {
      super(c);
   }

   @Override
   public MFGSwingWorker Copy() {
      WorkerStatHypoM ret = new WorkerStatHypoM(container);
      ret.resultField = resultField;
      ret.progressBar = progressBar;
      ret.enable = enable;
      return ret;
   }

   @Override
   public void SetAppearance() {
      enable.setText("Hypothesis");
   }

   @Override
   protected String doInBackground() {
      boolean[][] ignm = container.GetFullIgnoreMap();
      Integer[][] m = container.GetMaximalConf();
      Integer[][] mfilled = BMUtil.GreedyFillMFG(m,container.Height(),container.Width());
      boolean res = BMUtil.HasDiagRect_naive(mfilled,container.Height(),container.Width());
      return !res ? "Works" : "Fails";
   }
}
