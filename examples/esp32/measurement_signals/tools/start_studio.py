"""Open the sibling development Studio with an isolated numeric demo workspace."""
import os
import argparse
from pathlib import Path
import shutil
import sys
import tempfile

EXAMPLE = Path(__file__).resolve().parents[1]
STUDIO = EXAMPLE.parents[2].parent/'KymoStudio'
SESSION = Path(tempfile.mkdtemp(prefix='kymo-measurement-live-'))
shutil.copy2(EXAMPLE/'kymostudio.json', SESSION/'config.json')
os.environ['KYMO_CONFIG_HOME'] = str(SESSION)
sys.path.insert(0, str(STUDIO/'python'))
from PySide6.QtCore import QTimer, QObject
from PySide6.QtQuick import QQuickWindow
from Studio.studio import KymoStudio
from main import main

original_load = KymoStudio.load_app


def load_and_open_analysis(studio):
    original_load(studio)
    if studio.rootObjects():
        def open_analysis():
            window = studio.rootObjects()[0].findChild(QObject, 'measurementAnalysis')
            if window:
                window.show()
                window.raise_()
                window.requestActivate()
        QTimer.singleShot(2000, open_analysis)


KymoStudio.load_app = load_and_open_analysis
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--no-connect', action='store_true', help='Open the workspace while leaving the board port free')
    options = parser.parse_args()
    sys.exit(main(['--config', str(SESSION/'config.json'), *([] if options.no_connect else ['--start'])]))
