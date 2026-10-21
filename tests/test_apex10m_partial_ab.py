"""Focused safety tests for the partial APEX 10M A/B listener."""
import pathlib
import sys
import unittest
import numpy as np
import torch

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"apex_engine"))
from apex10m_partial_ab import real_features, process
from apex10m_model import SpatialReasoner

class PartialPreviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        torch.set_num_threads(2)
        torch.manual_seed(719)
        cls.model=SpatialReasoner().eval()
        cls.rate=48000
        t=np.arange(24000,dtype=np.float32)/cls.rate
        center=.19*np.sin(2*np.pi*60*t)+.13*np.sin(2*np.pi*400*t)
        side=.09*np.sin(2*np.pi*3400*t)
        cls.stereo=np.column_stack([center+side,center-side]).astype(np.float32)

    def test_feature_shapes_ranges(self):
        features,frame=real_features(self.stereo,self.rate)
        self.assertEqual(features.shape,(8,5))
        self.assertGreater(frame,1000)
        self.assertTrue(np.isfinite(features).all())
        self.assertTrue(np.all(features[3]>=-1))
        self.assertTrue(np.all(features[3]<=1))

    def test_bypass_and_mono(self):
        bypass,_=process(self.stereo,self.rate,self.model,0.0)
        self.assertTrue(np.array_equal(bypass,self.stereo))
        mono=np.column_stack([self.stereo[:,0],self.stereo[:,0]])
        output,_=process(mono,self.rate,self.model,1.0)
        self.assertTrue(np.array_equal(output,mono))

    def test_antiphase_guard(self):
        signal=self.stereo[:,0]
        anti=np.column_stack([signal,-signal]).astype(np.float32)
        output,report=process(anti,self.rate,self.model,1.0)
        self.assertTrue(np.array_equal(output,anti))
        self.assertIn("antiphase",report["reason"])

    def test_real_model_processing_output_stays_bounded(self):
        output,report=process(self.stereo,self.rate,self.model,.75)
        self.assertEqual(output.shape,self.stereo.shape)
        self.assertTrue(np.isfinite(output).all())
        self.assertLess(float(np.max(np.abs(output))),1.)
        self.assertLess(float(np.max(np.abs(output-self.stereo))),.08)
        self.assertIn("neural_spatial_decision_mean",report)
        self.assertFalse(report["real_music_quality_validated"])

if __name__=="__main__":
    unittest.main()
