package com.applications;

import javax.swing.*;
import java.awt.*;

public class MFGWorkerPanel {
   public MFGSwingWorker worker;
   public JCheckBox enable;
   public JTextPane resultField;
   public JProgressBar progressBar;
   public JPanel rightPanel;

   public MFGWorkerPanel(MFGSwingWorker w, JPanel parent)
   {
      JPanel pan = new JPanel();
      enable = new JCheckBox();
      enable.setSelected(true);
      w.enable = enable;
      resultField = new JTextPane();
      resultField.setPreferredSize(new Dimension(100,20));
      w.resultField = resultField;
      progressBar = new JProgressBar();
      w.progressBar = progressBar;
      pan.add(enable);
      pan.add(resultField);
      pan.add(progressBar);
      parent.add(pan);
      rightPanel = parent;
      w.subPanel = pan;
      worker = w;
      worker.SetAppearance();
   }

   public void PerformComputation()
   {
      worker.cancel(true);
      if(enable.isSelected())
      {
         worker = worker.Copy();
         worker.execute();
      }
      else
      {
         resultField.setText("");
      }
   }
}
