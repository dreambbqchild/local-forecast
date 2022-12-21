import * as dotenv from 'dotenv';
dotenv.config();

import express from 'express';
import { resolve } from 'path';
import htmlExpress, { staticIndexHandler } from 'html-express-js';

const __dirname = resolve();

const app = express();

app.engine(
  'js',
  htmlExpress({
    includesDir: 'includes',
  })
);

app.set('view engine', 'js');
app.set('views', `${__dirname}/public`);

// serve all other static files like CSS, images, etc
app.use(express.static(`${__dirname}/public`));

// Automatically serve any index.js file as HTML in the public directory
app.use(
  staticIndexHandler({
    viewsDir: `${__dirname}/public`
  })
);

const port = 8080;
app.listen(port, function () {
  console.log(`Server started at http://localhost:${port}`);
});