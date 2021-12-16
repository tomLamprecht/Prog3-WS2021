import {ItemModel} from './itemModel'

export interface ColumnModel{
  id: number;
  name: string;
  position: number;
  items: ItemModel[];
}
