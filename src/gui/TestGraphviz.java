package gui;

import framework.combinatorics.DFA;
import framework.combinatorics.Transition;
import guru.nidi.graphviz.attribute.*;
import guru.nidi.graphviz.engine.Format;
import guru.nidi.graphviz.engine.Graphviz;
import guru.nidi.graphviz.model.Graph;
import guru.nidi.graphviz.model.Node;


import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import static guru.nidi.graphviz.attribute.Attributes.attr;
import static guru.nidi.graphviz.attribute.Rank.RankDir.LEFT_TO_RIGHT;
import static guru.nidi.graphviz.model.Factory.*;

public class TestGraphviz {
   public static void main(String[] args) throws IOException {
      Graph g = graph("example1").directed()
              .graphAttr().with(Rank.dir(LEFT_TO_RIGHT))
              //.nodeAttr().with(Font.name("arial"))
              .linkAttr().with("class", "link-class")
              .with(
                      node("a").with(Color.RED).link(node("b")),
                      node("b").link(
                              to(node("c")).with(attr("weight", 5), Style.DASHED)
                      )
              );
      Graphviz.fromGraph(g).height(100).render(Format.PNG).toFile(new File("example/ex1.png"));

   }
   public static BufferedImage gettestimage(){
      Graph g = graph("example1").directed()
              .graphAttr().with(Rank.dir(LEFT_TO_RIGHT))
              //.nodeAttr().with(Font.name("arial"))
              .linkAttr().with("class", "link-class")
              .with(
                      node("a").with(Color.RED).link(node("b")),
                      node("b").link(
                              to(node("c")).with(attr("weight", 5), Style.DASHED)
                      )
              );
      return Graphviz.fromGraph(g).height(100).render(Format.PNG).toImage();
   }
   private static DFA getadfa(){
      DFA fa = new DFA(6,2);
      fa.SetTransitions(new Transition[] {
              new Transition(0,1,0),
              new Transition(0,2,1),
              new Transition(1,0,0),
              new Transition(1,3, 1),
              new Transition(2,4, 0),
              new Transition(2,5, 1),
              new Transition(3,4, 0),
              new Transition(3,5, 1),
              new Transition(4,4, 0),
              new Transition(4,5, 1),
              new Transition(5,5, 0),
              new Transition(5,5, 1),
      });
      fa.SetAcceptingState(2,true);
      fa.SetAcceptingState(3,true);
      fa.SetAcceptingState(4,true);
      fa = fa.Intersects(fa);
      fa = fa.Minimize();
      return fa;
   }

   public static Graph getgraphfromdfa(DFA d)
   {
      List<Node> ns = new ArrayList<>();
      for(int i = 0;i<d.statecount;++i)
      {
         Node n = node(String.valueOf(i));
         ns.add(n.link(
                 to(node(String.valueOf(d.transfunc[i][0]))).with(Label.of("0")),
                 to(node(String.valueOf(d.transfunc[i][1]))).with(Label.of("1"))
                 )
         );
      }
      Graph g = graph("result").directed()
              .linkAttr().with("class","link=class")
              .with(ns);
      return g;
   }
   public static BufferedImage testdfatograph() {
      return Graphviz.fromGraph(getgraphfromdfa(getadfa())).height(500).render(Format.PNG).toImage();
   }
}
