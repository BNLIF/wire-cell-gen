#!/usr/bin/env python

import ROOT

from array import array
stops = array('d',[ 0.00, 0.45, 0.50, 0.55, 1.00 ])
reds =  array('d',[ 0.00, 0.00, 1.00, 1.00, 0.51 ])
greens =array('d',[ 0.00, 0.81, 1.00, 0.20, 0.00 ])
blues  =array('d',[ 0.51, 1.00, 1.00, 0.00, 0.00 ])

#ROOT.gStyle.SetPalette(ROOT.kVisibleSpectrum)
ROOT.TColor.CreateGradientColorTable(len(stops), stops, reds, greens, blues, 100)
ROOT.gStyle.SetNumberContours(100)

fp = ROOT.TFile.Open("build/gen/test_impactzipper-uvw.root")
hists = {u:fp.Get("h%d"%n) for n,u in enumerate("uvw")}
limits = [6.0e-12, 6.0e-12, 15e-12]

c = ROOT.TCanvas()
c.SetRightMargin(0.15)
c.Print("test_impactzipper.pdf[","pdf")
c.SetGridx()
c.SetGridy()

for (p,h),lim in zip(sorted(hists.items()), limits):
    print p
    h.Draw("colz")
    h.SetTitle(h.GetTitle() + " point source")
    h.GetXaxis().SetRangeUser(3800, 3990)
    h.GetYaxis().SetRangeUser(989, 1012)
    h.GetZaxis().SetRangeUser(-lim, lim)
    c.Print("test_impactzipper.pdf","pdf")

c.Print("test_impactzipper.pdf]","pdf")
