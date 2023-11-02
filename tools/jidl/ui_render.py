# Copyright 2023 Xiaomi, Inc. All rights reserved.

import source_render as render

class UIUtils(render.Utils):
  def getNativeType(self, tp):
    return self.findTypeMeta(tp['type'], tp['name'], 'native_type', True)


if __name__ == '__main__':
    render.main(UIUtils)
