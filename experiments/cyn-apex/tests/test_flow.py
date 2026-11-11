import unittest
import numpy as np
from apex.flow import analyze_audio

SR=12000
def synth(strengths):
    t=np.arange(SR)/SR
    parts=[]
    for amplitude in strengths:
        x=amplitude*(.6*np.sin(2*np.pi*105*t)+.2*np.sin(2*np.pi*440*t))
        for onset in (0.0,.25,.5,.75):
            x += amplitude*.25*np.exp(-55*np.maximum(t-onset,0))*(t>=onset)*np.sin(2*np.pi*85*t)
        parts.append(x.astype("float32"))
    return np.concatenate(parts)

class FlowTests(unittest.TestCase):
    def test_constant_volume(self):
        result=analyze_audio(synth([.2]*8),SR)
        self.assertEqual(len(result["segments"]),8)
        self.assertFalse(any(e["kind"]=="impact_transition" for e in result["events"]))
    def test_energy_arc(self):
        result=analyze_audio(synth([.05,.07,.1,.14,.21,.31,.7,.03,.03,.03]),SR)
        self.assertGreater(result["segments"][6]["intensity"],result["segments"][0]["intensity"])
        self.assertTrue(any(e["kind"]=="breakdown" for e in result["events"]))
        self.assertTrue(any(e["kind"] in ("impact_transition","build") for e in result["events"]))
    def test_silence(self):
        result=analyze_audio(np.zeros(SR*6,dtype=np.float32),SR)
        self.assertIsNone(result["estimated_pulse_bpm"])
        self.assertEqual(result["pulse_confidence"],0)
    def test_invalid_inputs(self):
        for x in [np.ones(10),np.full(SR,np.nan),np.ones((SR,3))]:
            with self.assertRaises(ValueError):
                analyze_audio(x,SR)
    def test_stereo_and_resampling(self):
        signal=synth([.08]*6)
        self.assertEqual(analyze_audio(np.column_stack((signal,signal)),SR)["duration_s"],6)
        self.assertEqual(analyze_audio(signal[::2],SR//2)["duration_s"],6)

if __name__=="__main__":
    unittest.main()
