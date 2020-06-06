package com.applications;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

public class MFG {
   public static void main(String[] args) {
      try{
         UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
      }catch (Exception e)
      {

      }

      MFG instance = new MFG();

   }
   public MFG(){
      _buttonCache = new ArrayList<>();
      frame = new JFrame("MFG");
      frame.setContentPane(mainPanel);
      frame.addKeyListener(new KeyListener() {
         @Override
         public void keyTyped(KeyEvent e) {

         }

         @Override
         public void keyPressed(KeyEvent e) {

         }

         @Override
         public void keyReleased(KeyEvent e) {
            if(e.getKeyCode() == KeyEvent.VK_Z) {
               container.Revert();
               RefreshAllPanel();
            }
         }
      });
      frame.setFocusable(true);
      frame.requestFocusInWindow();
      frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      container = new BMContainer();
      compute.addActionListener(new ActionListener() {
         @Override
         public void actionPerformed(ActionEvent e) {
            computeAction();
         }
      });
      RefreshAllPanel();
      frame.setVisible(true);
   }
   private AtomicBoolean atomic = new AtomicBoolean(false);
   private Thread workingthread;
   private void computeAction() {
      if(atomic.get())
      {
         workingthread.interrupt();
         atomic.set(false);
      }
      atomic.set(true);
      result.setText("Computing");
      if(ratio.isSelected())
      {
         workingthread = new Thread(){
            public void run(){
               double r = 0.0;
               try {
                  r = container.RatioDRFfillingsOfMaxConf();
               } catch (Exception exception) {
                  //too big need to approximate.
               }
               result.setText(String.format("%.12f", r));
               atomic.set(false);
            }
         };
         workingthread.start();
      }
      else if(exact.isSelected())
      {
         workingthread = new Thread(){
            public void run(){
               long r = 0;
               try {
                  r = container.CountDRFfillingsOfMaxConf();
               } catch (Exception exception) {
                  //too big need to approximate.
               }
               result.setText(String.valueOf(r));
               atomic.set(false);
            }
         };
         workingthread.start();
      }
   }

   private JFrame frame;
   private JPanel mainPanel;
   private JPanel gridPanel;
   private JPanel rightPanel;
   private JPanel downPanel;
   private JRadioButton ratio;
   private JRadioButton exact;
   private JCheckBox updateEveryStep;
   private JTextArea result;
   private JButton compute;
   private BMContainer container;
   private ArrayList<JButton> _buttonCache;

   private void RefreshAllPanel()
   {
      RefreshMatPanel(gridPanel);
      if(updateEveryStep.isSelected())
         computeAction();
      frame.pack();
   }
   private void RefreshMatPanel(JPanel pane)
   {
      _buttonCache.clear();
      pane.removeAll();
      pane.setLayout(new GridBagLayout());
      GridBagConstraints gbc = new GridBagConstraints();
      Integer[][] m = container.GetMaximalConf();
      final int panelh = 500;
      final int panelw = 500;
      final int maxw = 50;
      final int maxh = 50;
      // side bars
      int h = container.Height();
      int w = container.Width();
      int hs = Math.min(maxh, (panelh-50)/(2*h+3));
      int ws = Math.min(maxw, (panelw-50)/(2*w+3));
      int rn = 2 * h + 2;
      int cn = 2 * w + 2;
      gbc.gridx = 0;
      gbc.ipadx = 50;
      gbc.ipady = hs;
      for(int i = 0;i<h+1;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(50,hs));
         b.setBackground(Color.red);
         b.setOpaque(true);
         b.setText("+");
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               JButton eb = (JButton)e.getSource();
               container.InsertRow(Integer.parseInt(eb.getName()));
               RefreshAllPanel();
            }
         });
         gbc.gridy = 2*i+1;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      for(int i = 0;i<h;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(50,hs));
         b.setBackground(Color.gray);
         b.setOpaque(true);
         b.setText("-");
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               if(container.Height()<=1)return;
               JButton eb = (JButton)e.getSource();
               container.RemoveRow(Integer.parseInt(eb.getName()));
               RefreshAllPanel();
            }
         });
         gbc.gridy = 2*i+2;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      gbc.gridy = 0;
      gbc.ipadx = ws;
      gbc.ipady = 50;
      for(int i = 0;i<w+1;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(ws,50));
         b.setBackground(Color.red);
         b.setOpaque(true);
         b.setText("+");
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               JButton eb = (JButton)e.getSource();
               container.InsertCol(Integer.parseInt(eb.getName()));
               RefreshAllPanel();
            }
         });
         gbc.gridx = 2*i+1;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      for(int i = 0;i<w;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(ws,50));
         b.setBackground(Color.gray);
         b.setOpaque(true);
         b.setText("-");
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               if(container.Width()<=1)return;
               JButton eb = (JButton)e.getSource();
               container.RemoveCol(Integer.parseInt(eb.getName()));
               RefreshAllPanel();
            }
         });
         gbc.gridx = 2*i+2;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      //matrix
      gbc.ipadx = ws;
      gbc.ipady = hs;
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
         {
            JButton b = new JButton();
            b.setPreferredSize(new Dimension(ws,hs));
            b.setName(Integer.toString(i)+","+Integer.toString(j));
            b.setFocusable(false);
            gbc.gridx = 2*j+2;
            gbc.gridy = 2*i+2;
            if(m[i][j] == 1)
            {
               b.setText("1");
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,2);
                     RefreshAllPanel();
                  }
               });
            }
            else if(m[i][j] == 0)
            {
               b.setText("0");
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,1);
                     RefreshAllPanel();
                  }
               });
            }
            else if(m[i][j]==2)
            {
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,1);
                     RefreshAllPanel();
                  }
               });
            }
            _buttonCache.add(b);
            pane.add(b,gbc);
         }
      //pane.revalidate();
      pane.repaint();
   }

}
